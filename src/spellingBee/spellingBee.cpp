#include <string>
#include <vector>
#include <array>
#include <bitset>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cctype>
#include <unordered_set>

#include "spellingBee.hpp"

namespace SpellingBee
{
    bool isValidWord(Utils::Word &word, const Config &config)
    {
        if (word.wordString.size() <= 3)
            return false;

        bool hasMiddleLetter = false;
        for (char c : word.wordString)
        {
            if (!config.validLettersMap[static_cast<unsigned char>(c)])
                return false;
            hasMiddleLetter |= (c == config.allLetters[0]);
        }
        return hasMiddleLetter;
    }

    void filterWords(std::vector<Utils::Word> &words, const Config &config)
    {
        words.erase(
            std::remove_if(words.begin(), words.end(),
                           [&](Utils::Word &word)
                           { return !isValidWord(word, config); }),
            words.end());
    }

    std::vector<Utils::Word> runSpellingBeeSolver(const std::vector<Utils::Word> &words, const Config &config)
    {
        std::vector<Utils::Word> wordsCopy = words;
        filterWords(wordsCopy, config);

        std::sort(wordsCopy.begin(), wordsCopy.end(), [](const Utils::Word &a, const Utils::Word &b)
                  {
                      if (a.uniqueLetters != b.uniqueLetters)
                          return a.uniqueLetters > b.uniqueLetters;
                      if (a.score != b.score)
                          return a.score > b.score;
                      return a.wordString < b.wordString; // Sort by text last
                  });
        return wordsCopy;
    }
}