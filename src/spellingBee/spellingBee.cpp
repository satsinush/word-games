#include <algorithm>
#include <array>
#include <bitset>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "spellingBee/spellingBee.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

namespace SpellingBee {
bool isValidWord(Utils::Word &word, const Config &config) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  if (config.excludeUncommonWords && !word.is_scrabble)
    return false;

  if (word.wordString.size() <= 3)
    return false;

  // Check if first letter is required and present
  bool hasFirstLetter = false;
  if (!config.allLetters.empty()) {
    for (char c : word.wordString) {
      if (c == config.allLetters[0]) {
        hasFirstLetter = true;
        break;
      }
    }
  }

  if (config.mustIncludeFirstLetter && !config.allLetters.empty() &&
      !hasFirstLetter)
    return false;

  // Check letter validity
  if (config.reuseLetters) {
    // Allow reuse of letters
    for (char c : word.wordString) {
      if (!config.validLettersMap[static_cast<unsigned char>(c)])
        return false;
    }
  } else {
    // Check that word doesn't use more of any letter than available
    std::array<int, 256> letterCounts = {0};
    std::array<int, 256> availableCounts = {0};

    // Count available letters
    for (char c : config.allLetters) {
      availableCounts[static_cast<unsigned char>(c)]++;
    }

    // Count letters in word
    for (char c : word.wordString) {
      if (!config.validLettersMap[static_cast<unsigned char>(c)])
        return false;
      letterCounts[static_cast<unsigned char>(c)]++;
      if (letterCounts[static_cast<unsigned char>(c)] >
          availableCounts[static_cast<unsigned char>(c)])
        return false;
    }
  }

  return true;
}

void filterWords(std::vector<Utils::Word> &words, const Config &config) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  words.erase(std::remove_if(words.begin(), words.end(),
                             [&](Utils::Word &word) {
                               return !isValidWord(word, config);
                             }),
              words.end());
}

Result runSpellingBeeSolver(const Config &config, std::atomic<bool> *cancel) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  std::vector<Utils::Word> wordsCopy = Utils::loadWords();
  filterWords(wordsCopy, config);

  if (cancel && cancel->load()) {
    return {{}, 0};
  }

  std::sort(wordsCopy.begin(), wordsCopy.end(),
            [](const Utils::Word &a, const Utils::Word &b) {
              if (a.uniqueLetters != b.uniqueLetters)
                return a.uniqueLetters > b.uniqueLetters;
              return a < b; // Sort by word last
            });

  Result result;
  result.words = std::move(wordsCopy);
  result.totalValidWords = static_cast<int>(result.words.size());
  return result;
}
} // namespace SpellingBee