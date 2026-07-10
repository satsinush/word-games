#pragma once
#include <array>
#include <cmath>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "utils/EntSolver.hpp"
#include "utils/inputUtils.hpp"
#include "utils/utils.hpp"
#include <atomic>

namespace Mastermind {

struct Feedback; // forward declaration so Config can reference Feedback

struct Config {
  uint8_t numPegs = 4;               // Number of pegs in the pattern
  std::string colorChars = "RGBCMY"; // Available color characters (max 256)
  bool allowDuplicates = true;       // Whether duplicate colors are allowed
  uint8_t maxDepth = 1;              // How many moves ahead to calculate ENT
  bool autoDepth = false;            // Dynamically choose optimal depth
  uint32_t maxGuesses = 10;          // Maximum allowed guesses
  std::vector<Feedback> feedbackHistory = {}; // History of previous feedbacks

  // Helper to get number of colors
  uint8_t numColors() const {
    return static_cast<uint8_t>(colorChars.length());
  }

  // Helper to convert character to color index
  int charToColor(char c) const {
    size_t pos = colorChars.find(c);
    return (pos != std::string::npos) ? static_cast<int>(pos) : -1;
  }

  // Helper to convert color index to character
  char colorToChar(uint8_t color) const {
    return (color < colorChars.length()) ? colorChars[color] : '?';
  }
};

static constexpr size_t MAX_PEGS = 32; // Maximum number of pegs supported

struct Pattern {
  std::array<uint8_t, MAX_PEGS> colors; // Array of color values
  uint8_t numPegs = 4;                  // Actual number of pegs used
  double score = 1.0;                   // Score for weighting (default 1.0)
  std::array<uint8_t, 256> colorCount =
      {}; // Precomputed color occurrence counts

  Pattern() { colors.fill(0); }
  Pattern(const std::array<uint8_t, MAX_PEGS> &c, uint8_t pegCount)
      : numPegs(pegCount) {
    colors = c;
    computeColorCount();
  }
  Pattern(uint8_t pegCount) : numPegs(pegCount) { colors.fill(0); }

  // Compute color occurrence counts
  void computeColorCount() {
    colorCount.fill(0);
    for (uint8_t i = 0; i < numPegs; ++i) {
      colorCount[colors[i]]++;
    }
  }

  bool operator<(const Pattern &other) const {
    if (numPegs != other.numPegs)
      return numPegs < other.numPegs;
    for (uint8_t i = 0; i < numPegs; ++i) {
      if (colors[i] != other.colors[i])
        return colors[i] < other.colors[i];
    }
    return false;
  }

  bool operator==(const Pattern &other) const {
    if (numPegs != other.numPegs)
      return false;
    for (size_t i = 0; i < numPegs; ++i) {
      if (colors[i] != other.colors[i])
        return false;
    }
    return true;
  }

  std::string toString(const Config &config) const {
    std::string result;
    for (uint8_t i = 0; i < numPegs; ++i) {
      result += config.colorToChar(colors[i]);
    }
    return result;
  }
};

struct Feedback {
  Pattern guess; // The guessed pattern
  uint8_t correctPosition =
      0; // Number of pegs in correct position (like green in wordle)
  uint8_t correctColor =
      0; // Number of colors correct but wrong position (like yellow in wordle)

  bool operator==(const Feedback &other) const {
    return guess == other.guess && correctPosition == other.correctPosition &&
           correctColor == other.correctColor;
  }

  bool operator<(const Feedback &other) const {
    if (guess != other.guess)
      return guess < other.guess;
    if (correctPosition != other.correctPosition)
      return correctPosition < other.correctPosition;
    return correctColor < other.correctColor;
  }
};

struct PatternGuess {
  Pattern pattern;
  double ent = 0.0; // Expected Number of Turns
  double wnt = 0.0; // Worst Number of Turns
  double probability = 0.0;

  bool operator<(const PatternGuess &other) const {
    // Primary sort: Expected Number of Turns (lower is better)
    const double tolerance = 1e-9;

    if (std::abs(ent - other.ent) > tolerance)
      return ent < other.ent; // Sort lower ENT first (fewer expected turns)

    // Secondary tiebreaker: probability (higher is better - prefer possible
    // answers)
    if (std::abs(probability - other.probability) > tolerance)
      return probability > other.probability; // Sort higher probability first

    // Third tiebreaker: WNT (lower is better)
    if (std::abs(wnt - other.wnt) > tolerance)
      return wnt < other.wnt;

    // Final tiebreaker: sort by pattern for consistency
    return pattern < other.pattern;
  }
};

struct Result {
  std::vector<PatternGuess> sortedGuesses;
  int totalPossiblePatterns = 0;
};

// Parse feedback string like "rgbc 2 1" (pattern correctPos correctCol)
Feedback parseFeedback(const std::string &input, const Config &config);

// Check if a pattern matches feedback constraints
bool matchesFeedback(const Pattern &candidate, const Feedback &fb);

// Generate feedback for a guess against a target pattern
Feedback generateFeedback(const Pattern &target, const Pattern &guess);

// Generate all possible patterns for the given configuration
std::vector<Pattern> generateAllPatterns(const Config &config);

// Generic EntSolver-based version (cleaner implementation)
Result runMastermindSolver(const Config &config = Config{},
                           std::atomic<bool> *cancel = nullptr);
} // namespace Mastermind

// Provide std::hash specializations for Mastermind types so they can be used as
// unordered_map/unordered_set keys in generic code (like the EntSolver).
namespace std {
// Improved hash function for Mastermind::Pattern
// Uses MurmurHash3 for better distribution
template <> struct hash<Mastermind::Pattern> {
  size_t operator()(const Mastermind::Pattern &pattern) const noexcept {
    uint64_t h = 1469598103934665603ULL; // FNV-1a offset basis
    for (uint8_t i = 0; i < pattern.numPegs; ++i) {
      h ^= static_cast<uint64_t>(pattern.colors[i]);
      h *= 1099511628211ULL; // FNV-1a prime
    }
    return static_cast<size_t>(h);
  }
};

// Improved hash function for Mastermind::Feedback
// Combines guess colors, correctPosition, and correctColor
// Uses MurmurHash3 for better distribution
template <> struct hash<Mastermind::Feedback> {
  size_t operator()(const Mastermind::Feedback &fb) const noexcept {
    uint64_t h = 1469598103934665603ULL; // FNV-1a offset basis
    for (uint8_t i = 0; i < fb.guess.numPegs; ++i) {
      h ^= static_cast<uint64_t>(fb.guess.colors[i]);
      h *= 1099511628211ULL; // FNV-1a prime
    }

    // Mix in correctPosition and correctColor
    h ^= static_cast<uint64_t>(fb.correctPosition) + 0x9e3779b97f4a7c15ULL +
         (h << 6) + (h >> 2);
    h ^= static_cast<uint64_t>(fb.correctColor) + 0x9e3779b97f4a7c15ULL +
         (h << 6) + (h >> 2);

    return static_cast<size_t>(h);
  }
};
} // namespace std
