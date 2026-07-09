#pragma once
#include <array>
#include <bitset>
#include <cmath>
#include <string>
#include <vector>

#include "utils/EntSolver.hpp"
#include "utils/inputUtils.hpp"
#include "utils/utils.hpp"
#include <atomic>

namespace Wordle {

struct Feedback; // forward declaration so Config can reference Feedback

struct Config {
  uint8_t maxDepth = 1; // How many moves ahead to calculate ENT
  bool autoDepth = false; // Dynamically choose optimal depth
  bool excludeUncommonWords = false;
  uint8_t wordLength = 5; // Length of words to use (default 5)
  std::vector<Feedback> feedbackHistory = {};
};

struct Feedback {
  std::string word;       // Variable-length guess
  std::bitset<64> colors; // Support up to 32-letter words (32*2=64 bits)
                          // bits 0,2,4,... = letter in word
                          // bits 1,3,5,... = correct position

  bool operator==(const Feedback &other) const {
    return word == other.word && colors == other.colors;
  }

  bool operator<(const Feedback &other) const {
    if (word != other.word)
      return word < other.word;
    return colors.to_ullong() < other.colors.to_ullong();
  }

  // Helper methods to get/set feedback for position i
  void setGrey(const int i) {
    colors.reset(i * 2);
    colors.reset(i * 2 + 1);
  }

  void setYellow(const int i) {
    colors.set(i * 2);
    colors.reset(i * 2 + 1);
  }

  void setGreen(const int i) {
    colors.set(i * 2);
    colors.set(i * 2 + 1);
  }

  int getColor(const int i) const {
    if (colors[i * 2 + 1])
      return 2; // green
    if (colors[i * 2])
      return 1; // yellow
    return 0;   // grey
  }
};

struct WordGuess {
  Utils::Word word;
  double ent = 0.0; // Expected Number of Turns
  double probability = 0.0;

  bool operator<(const WordGuess &other) const {
    // Primary sort: Expected Number of Turns (lower is better)
    const double tolerance = 1e-9;

    if (std::abs(ent - other.ent) > tolerance)
      return ent < other.ent; // Sort lower ENT first (fewer expected turns)

    // Secondary tiebreaker: probability (higher is better - prefer possible
    // answers)
    if (std::abs(probability - other.probability) > tolerance)
      return probability > other.probability; // Sort higher probability first

    // Final tiebreaker: sort by word for consistency
    return word < other.word;
  }
};

struct Result {
  std::vector<WordGuess> sortedGuesses;
  int totalPossibleWords = 0;
};

// Parse feedback string like "STEAL 01201"
Feedback parseFeedback(const std::string &input);

// Check if a word matches feedback constraints
bool matchesFeedback(const Utils::Word &candidate, const Feedback &fb);

// Generate feedback for a guess against a target word
Feedback generateFeedback(const Utils::Word &target, const std::string &guess);

// Get all possible feedback patterns for a 5-letter word
std::vector<Feedback> getAllPossibleFeedbacks();

// Filter possible words given a list of guesses and feedbacks
std::vector<Utils::Word> filterWords(const std::vector<Utils::Word> &words,
                                     const std::vector<Feedback> &feedbacks);

// Generic EntSolver-based version (cleaner implementation)
Result runWordleSolver(const Config &config = Config{},
                       std::atomic<bool> *cancel = nullptr);
} // namespace Wordle

// Provide std::hash specialization for Wordle::Feedback so it can be used as an
// unordered_map/unordered_set key in generic code (like the EntSolver).
namespace std {
// Improved hash function for Wordle::Feedback
// Handles cases where colors exceed 64 bits
template <> struct hash<Wordle::Feedback> {
  size_t operator()(const Wordle::Feedback &fb) const noexcept {
    size_t h1 = std::hash<std::string>{}(fb.word);

    // Use a custom hash for the bitset to handle larger sizes
    size_t h2 = 0;
    for (size_t i = 0; i < fb.colors.size(); ++i) {
      h2 ^= (fb.colors[i] + 0x9e3779b9 + (h2 << 6) + (h2 >> 2));
    }

    // Combine the two hashes
    return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
  }
};
} // namespace std