#include "hangman/hangman.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

#include "utils/EntSolver.hpp"

namespace Hangman {

std::vector<WordPattern> parsePatternString(const std::string &patternStr) {
  std::vector<WordPattern> patterns;
  std::istringstream iss(patternStr);
  std::string token;

  while (iss >> token) {
    if (!token.empty()) {
      WordPattern wp;
      // Normalize: convert to uppercase for display consistency, but store
      // lowercase internally
      for (char &c : token) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
          c = static_cast<char>(std::tolower(c));
        }
      }
      wp.pattern = token;
      patterns.push_back(wp);
    }
  }

  return patterns;
}

std::string patternsToString(const std::vector<WordPattern> &patterns) {
  std::string result;
  for (size_t i = 0; i < patterns.size(); ++i) {
    if (i > 0)
      result += " ";
    // Convert to uppercase for display
    std::string upper = patterns[i].pattern;
    for (char &c : upper) {
      c = static_cast<char>(std::toupper(c));
    }
    result += upper;
  }
  return result;
}

Feedback parseFeedback(const std::string &input) {
  std::istringstream iss(input);
  char letter;
  int inWord;

  if (!(iss >> letter >> inWord)) {
    throw std::invalid_argument(
        "Invalid feedback format. Expected: 'a 1' or 'e 0'");
  }

  Feedback fb;
  fb.letter = static_cast<char>(std::tolower(letter));
  fb.isInWord = (inWord != 0);
  fb.occurrences = fb.isInWord ? 1 : 0;

  return fb;
}

std::vector<Feedback> parseStrikes(const std::string &strikes) {
  std::vector<Feedback> feedbackList;
  for (char c : strikes) {
    if (std::isalpha(static_cast<unsigned char>(c))) {
      Feedback fb;
      fb.letter = static_cast<char>(std::tolower(c));
      fb.isInWord = false;
      fb.occurrences = 0;
      feedbackList.push_back(fb);
    }
  }
  return feedbackList;
}

bool matchesPattern(const Utils::Word &word, const WordPattern &pattern) {
  // Word must have same length as pattern
  if (word.wordString.length() != pattern.length()) {
    return false;
  }

  // Check that revealed letters match
  for (const auto &[pos, letter] : pattern.getRevealedLetters()) {
    if (pos >= word.wordString.length())
      return false;
    if (std::tolower(word.wordString[pos]) != letter)
      return false;
  }

  return true;
}

bool matchesFeedback(const PhraseSolution &phrase, const Feedback &fb) {
  // Count total occurrences of the letter across all words
  size_t totalOccurrences = 0;
  for (const auto &word : phrase.words) {
    totalOccurrences += word.letterCount[fb.letter - 'a'];
  }

  bool letterInPhrase = (totalOccurrences > 0);
  return letterInPhrase == fb.isInWord;
}

bool matchesWordFeedback(const Utils::Word &word, const Feedback &fb) {
  bool letterInWord = (word.letterCount[fb.letter - 'a'] > 0);
  return letterInWord == fb.isInWord;
}

Feedback generateWordFeedback(const Utils::Word &target, char letter) {
  Feedback fb;
  fb.letter = static_cast<char>(std::tolower(letter));
  fb.occurrences = target.letterCount[fb.letter - 'a'];
  fb.isInWord = (fb.occurrences > 0);
  return fb;
}

Feedback generateFeedback(const PhraseSolution &target, char letter) {
  Feedback fb;
  fb.letter = static_cast<char>(std::tolower(letter));

  // Count total occurrences across all words
  size_t totalOccurrences = 0;
  for (const auto &word : target.words) {
    totalOccurrences += word.letterCount[fb.letter - 'a'];
  }

  fb.occurrences = totalOccurrences;
  fb.isInWord = (totalOccurrences > 0);

  return fb;
}

std::vector<char> getAllLetters() {
  std::vector<char> letters;
  letters.reserve(26);
  for (char c = 'a'; c <= 'z'; ++c) {
    letters.push_back(c);
  }
  return letters;
}

std::vector<char>
getAvailableLetters(const std::vector<Feedback> &feedbackHistory) {
  std::unordered_set<char> guessedLetters;
  for (const auto &fb : feedbackHistory) {
    guessedLetters.insert(fb.letter);
  }

  std::vector<char> available;
  available.reserve(26 - guessedLetters.size());
  for (char c = 'a'; c <= 'z'; ++c) {
    if (guessedLetters.find(c) == guessedLetters.end()) {
      available.push_back(c);
    }
  }
  return available;
}

// Filter words that match a pattern and feedback history
std::vector<Utils::Word>
filterWordsForPattern(const std::vector<Utils::Word> &words,
                      const WordPattern &pattern,
                      [[maybe_unused]] const std::vector<Feedback> &feedbacks) {

  std::vector<Utils::Word> filtered;

  for (const auto &word : words) {
    // Must match pattern length and revealed letters
    if (!matchesPattern(word, pattern)) {
      continue;
    }

    // For single-word filtering, we need a different approach
    // We'll filter phrases later - for now just match pattern
    filtered.push_back(word);
  }

  return filtered;
}

// Hangman-specific ENT solver implementation
// Uses WordSlotSolution as the solution type to track (slotIndex, word) pairs
// This avoids combinatorial explosion when dealing with multi-word phrases
class HangmanEntSolver
    : public Utils::AbstractEntSolver<WordSlotSolution, char, Feedback, Config,
                                      LetterGuess, Result> {
public:
  HangmanEntSolver(const Config &cfg, size_t numSlots,
                   const std::vector<std::vector<Utils::Word>> &wordsPerSlot)
      : Utils::AbstractEntSolver<WordSlotSolution, char, Feedback, Config,
                                 LetterGuess, Result>(cfg),
        numWordSlots(numSlots), wordsPerSlot_(wordsPerSlot) {}

protected:
  bool matchesFeedback(const WordSlotSolution &candidate,
                       const Feedback &feedback) const override {
    return Hangman::matchesWordFeedback(candidate.word, feedback);
  }

  Feedback generateFeedback(const WordSlotSolution &target,
                            const char &guess) const override {
    return Hangman::generateWordFeedback(target.word, guess);
  }

  LetterGuess createGuess(const char &letter, double ent,
                          double probability) const override {
    LetterGuess guess;
    guess.letter = letter;
    guess.ent = ent;
    guess.probability = probability;
    return guess;
  }

  Result createResult(const std::vector<LetterGuess> &guesses,
                      int totalPossible) const override {
    Result result;
    result.sortedGuesses = guesses;
    result.totalPossibleWords = totalPossible;
    return result;
  }

  double worstCaseExpectedTurns(size_t numCandidates) const override {
    // For multi-word hangman, we're "done" when we have exactly numWordSlots
    // candidates (one per slot). Adjust the estimate accordingly.
    if (numCandidates <= numWordSlots) {
      return 0.0; // Already solved (one word per slot)
    }
    // Estimate: log2 of effective remaining choices per slot
    double effectiveRemaining =
        static_cast<double>(numCandidates) / static_cast<double>(numWordSlots);
    return std::log2(effectiveRemaining);
  }

  // Override to calculate probability that this letter appears in ANY word slot
  // Uses: P(in phrase) = 1 - ∏(1 - P(in slot_i))
  double calculateGuessProbability(
      const char &letter,
      const std::unordered_set<WordSlotSolution> &possibleSolutions,
      [[maybe_unused]] double possibleProb) const override {

    // Count words containing this letter for each slot
    std::vector<int> containsLetterPerSlot(numWordSlots, 0);
    std::vector<int> totalPerSlot(numWordSlots, 0);

    for (const auto &slot : possibleSolutions) {
      totalPerSlot[slot.slotIndex]++;
      if (slot.word.letterCount[letter - 'a'] > 0) {
        containsLetterPerSlot[slot.slotIndex]++;
      }
    }

    // Calculate P(NOT in phrase) = ∏(1 - P(in slot_i))
    double probNotInPhrase = 1.0;
    for (size_t i = 0; i < numWordSlots; ++i) {
      if (totalPerSlot[i] > 0) {
        double probInSlot = static_cast<double>(containsLetterPerSlot[i]) /
                            static_cast<double>(totalPerSlot[i]);
        probNotInPhrase *= (1.0 - probInSlot);
      }
    }

    // P(in phrase) = 1 - P(NOT in phrase)
    return 1.0 - probNotInPhrase;
  }

  // A letter "solves" the game if it's the last unknown letter needed
  // For simplicity, we never consider a single letter as solving the game
  bool isGuessSolution(const char &guess,
                       const WordSlotSolution &solution) const override {
    (void)guess;
    (void)solution;
    return false; // Individual letters don't solve hangman
  }

  double getSolutionScore(const WordSlotSolution &solution) const override {
    return solution.word.score;
  }

  // Override to define "solved" as having exactly one word per slot
  bool isSolvedState(
      const std::vector<WordSlotSolution> &currentSolutions) const override {
    if (currentSolutions.size() != numWordSlots) {
      return false;
    }

    // Check that we have exactly one solution per slot
    std::vector<int> countPerSlot(numWordSlots, 0);
    for (const auto &sol : currentSolutions) {
      if (sol.slotIndex < numWordSlots) {
        countPerSlot[sol.slotIndex]++;
      }
    }

    for (size_t i = 0; i < numWordSlots; ++i) {
      if (countPerSlot[i] != 1) {
        return false;
      }
    }
    return true;
  }

private:
  size_t numWordSlots;
  const std::vector<std::vector<Utils::Word>> &wordsPerSlot_;
};

Result runHangmanSolver(const Config &config, std::atomic<bool> *cancel) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  Result result;

  // If no patterns specified, return empty result
  if (config.wordPatterns.empty()) {
    result.totalPossibleWords = 0;
    return result;
  }

  std::vector<Utils::Word> allWords = Utils::loadWords();
  size_t numSlots = config.wordPatterns.size();

  // Build list of possible words for each slot (word position)
  std::vector<std::vector<Utils::Word>> wordsPerSlot(numSlots);

  for (size_t slotIdx = 0; slotIdx < numSlots; ++slotIdx) {
    const auto &pattern = config.wordPatterns[slotIdx];
    for (const auto &word : allWords) {
      bool exclude = config.excludeUncommonWords && (!word.is_scrabble);
      if (exclude)
        continue;

      if (matchesPattern(word, pattern)) {
        // Check feedback history for this word
        bool matches = true;
        for (const auto &fb : config.feedbackHistory) {
          if (!matchesWordFeedback(word, fb)) {
            matches = false;
            break;
          }
        }
        if (matches) {
          wordsPerSlot[slotIdx].push_back(word);
        }
      }
    }
  }

  // Build slot solutions (one entry per word per slot)
  std::vector<WordSlotSolution> slotSolutions;
  std::unordered_set<std::string> uniqueWordsForDisplay;

  for (size_t slotIdx = 0; slotIdx < numSlots; ++slotIdx) {
    for (const auto &word : wordsPerSlot[slotIdx]) {
      WordSlotSolution slot;
      slot.slotIndex = slotIdx;
      slot.word = word;
      slot.score = word.score;
      slotSolutions.push_back(slot);

      // Track unique words for display
      uniqueWordsForDisplay.insert(word.wordString);
    }
  }

  // Get available letters (not yet guessed, and not already revealed in
  // patterns)
  std::vector<char> availableLetters =
      getAvailableLetters(config.feedbackHistory);

  // Also exclude letters that are already revealed in patterns
  std::unordered_set<char> revealedLetters;
  for (const auto &pattern : config.wordPatterns) {
    for (const auto &[pos, letter] : pattern.getRevealedLetters()) {
      revealedLetters.insert(letter);
    }
  }

  // Remove revealed letters from available
  availableLetters.erase(std::remove_if(availableLetters.begin(),
                                        availableLetters.end(),
                                        [&revealedLetters](char c) {
                                          return revealedLetters.count(c) > 0;
                                        }),
                         availableLetters.end());

  // Use the specialized Hangman ENT solver with slot-based solutions
  HangmanEntSolver solver(config, numSlots, wordsPerSlot);
  result = solver.solve(availableLetters, slotSolutions, cancel);

  // Collect unique possible words for display (from all slots)
  std::vector<Utils::Word> possibleWords;
  for (const auto &word : allWords) {
    if (uniqueWordsForDisplay.count(word.wordString) > 0) {
      possibleWords.push_back(word);
    }
  }

  // Store possible words for display
  result.possibleWords = possibleWords;
  result.totalPossibleWords = static_cast<int>(possibleWords.size());

  // Sort by score (higher is better)
  std::sort(result.possibleWords.begin(), result.possibleWords.end(),
            [](const Utils::Word &a, const Utils::Word &b) {
              return a.score > b.score;
            });

  return result;
}

} // namespace Hangman
