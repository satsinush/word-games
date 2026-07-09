#pragma once
#include <bitset>
#include <cmath>
#include <string>
#include <vector>

#include "utils/EntSolver.hpp"
#include "utils/inputUtils.hpp"
#include "utils/utils.hpp"
#include <atomic>

namespace Hangman {

struct Feedback; // forward declaration so Config can reference Feedback

// Represents a word pattern like "_A__" where _ is unknown, letters are
// revealed
struct WordPattern {
  std::string pattern; // The pattern string (e.g., "_A__")
  size_t length() const { return pattern.length(); }

  // Get revealed letters as a map of position -> letter
  std::vector<std::pair<size_t, char>> getRevealedLetters() const {
    std::vector<std::pair<size_t, char>> revealed;
    for (size_t i = 0; i < pattern.length(); ++i) {
      char c = pattern[i];
      if (c != '_' && std::isalpha(static_cast<unsigned char>(c))) {
        revealed.emplace_back(i, static_cast<char>(std::tolower(c)));
      }
    }
    return revealed;
  }

  bool operator==(const WordPattern &other) const {
    return pattern == other.pattern;
  }
};

// Represents a multi-word phrase solution (vector of words)
struct PhraseSolution {
  std::vector<Utils::Word> words;
  double score = 0.0;    // Combined score (sum)
  double minScore = 0.0; // Minimum word score in the phrase

  bool operator==(const PhraseSolution &other) const {
    if (words.size() != other.words.size())
      return false;
    for (size_t i = 0; i < words.size(); ++i) {
      if (words[i].wordString != other.words[i].wordString)
        return false;
    }
    return true;
  }

  std::string toString() const {
    std::string result;
    for (size_t i = 0; i < words.size(); ++i) {
      if (i > 0)
        result += " ";
      result += words[i].wordString;
    }
    return result;
  }
};

// Represents a word in a specific slot/position for the solver
// This allows us to track possibilities per word position without combinatorial
// explosion
struct WordSlotSolution {
  size_t slotIndex;   // Which word position (0, 1, 2, ...)
  Utils::Word word;   // The word candidate
  double score = 1.0; // Word score for probability weighting

  bool operator==(const WordSlotSolution &other) const {
    return slotIndex == other.slotIndex &&
           word.wordString == other.word.wordString;
  }

  std::string toString() const {
    return "[" + std::to_string(slotIndex) + "]:" + word.wordString;
  }
};

struct Config {
  uint8_t maxDepth = 1; // How many moves ahead to calculate ENT
  bool autoDepth = false; // Dynamically choose optimal depth
  bool excludeUncommonWords = false;
  std::vector<WordPattern> wordPatterns =
      {}; // Patterns for each word (e.g., {"_A__", "_A_", "_____"})
  std::vector<Feedback> feedbackHistory = {};
};

// Represents a letter guess and its feedback
struct Feedback {
  char letter;        // The guessed letter (lowercase)
  bool isInWord;      // Whether the letter is in any word
  size_t occurrences; // Total occurrences across all words (0 if not in phrase)

  bool operator==(const Feedback &other) const {
    return letter == other.letter && isInWord == other.isInWord;
  }

  bool operator<(const Feedback &other) const {
    if (letter != other.letter)
      return letter < other.letter;
    return isInWord < other.isInWord;
  }
};

// Represents a single letter as a guess input
struct LetterGuess {
  char letter;              // The letter (lowercase a-z)
  double ent = 0.0;         // Expected Number of Turns
  double probability = 0.0; // Probability this letter appears in the word

  bool operator<(const LetterGuess &other) const {
    const double tolerance = 1e-9;

    // Primary sort: Probability (higher is better)
    if (std::abs(probability - other.probability) > tolerance)
      return probability > other.probability;

    // Secondary sort: Expected Number of Turns (lower is better)
    if (std::abs(ent - other.ent) > tolerance)
      return ent < other.ent;

    // Final tiebreaker: sort by letter for consistency
    return letter < other.letter;
  }
};

struct Result {
  std::vector<LetterGuess> sortedGuesses; // All available letters ranked
  int totalPossibleWords = 0;             // Number of unique possible words
  std::vector<Utils::Word>
      possibleWords; // Unique words matching any pattern position
};

// Parse a pattern string like "_A__ _A_ _____" into WordPatterns
std::vector<WordPattern> parsePatternString(const std::string &patternStr);

// Convert patterns to a display string
std::string patternsToString(const std::vector<WordPattern> &patterns);

// Parse feedback string like "a 1" or "e 0" (letter and whether it's in word)
Feedback parseFeedback(const std::string &input);

// Parse strikes string - letters that are NOT in the word (e.g., "etxzq")
std::vector<Feedback> parseStrikes(const std::string &strikes);

// Check if a phrase matches all feedback constraints
bool matchesFeedback(const PhraseSolution &phrase, const Feedback &fb);

// Check if a single word matches feedback constraint
bool matchesWordFeedback(const Utils::Word &word, const Feedback &fb);

// Check if a word matches a pattern (considering revealed letters)
bool matchesPattern(const Utils::Word &word, const WordPattern &pattern);

// Generate feedback for a letter guess against a target phrase
Feedback generateFeedback(const PhraseSolution &target, char letter);

// Get all possible letters (a-z)
std::vector<char> getAllLetters();

// Filter already guessed letters from available guesses
std::vector<char>
getAvailableLetters(const std::vector<Feedback> &feedbackHistory);

// Run the Hangman solver with ENT-based algorithm
Result runHangmanSolver(const Config &config = Config{},
                        std::atomic<bool> *cancel = nullptr);

} // namespace Hangman

// Provide std::hash specialization for Hangman::Feedback
namespace std {
template <> struct hash<Hangman::Feedback> {
  size_t operator()(const Hangman::Feedback &fb) const noexcept {
    // Simple hash combining letter and isInWord flag
    size_t h1 = std::hash<char>{}(fb.letter);
    size_t h2 = std::hash<bool>{}(fb.isInWord);
    return h1 ^ (h2 << 1);
  }
};

template <> struct hash<Hangman::PhraseSolution> {
  size_t operator()(const Hangman::PhraseSolution &phrase) const noexcept {
    size_t hash = 0;
    for (const auto &word : phrase.words) {
      hash ^= std::hash<std::string>{}(word.wordString) + 0x9e3779b9 +
              (hash << 6) + (hash >> 2);
    }
    return hash;
  }
};

template <> struct hash<Hangman::WordSlotSolution> {
  size_t operator()(const Hangman::WordSlotSolution &slot) const noexcept {
    size_t h1 = std::hash<size_t>{}(slot.slotIndex);
    size_t h2 = std::hash<std::string>{}(slot.word.wordString);
    return h1 ^ (h2 << 1);
  }
};
} // namespace std
