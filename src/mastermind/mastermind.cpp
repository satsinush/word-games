#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "mastermind/mastermind.hpp"
#include "utils/EntSolver.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

namespace Mastermind {
Feedback parseFeedback(const std::string &input, const Config &config) {
  // Expected format: "abcde 1 2" (pattern as single string, then two numbers)
  std::istringstream iss(input);
  std::string patternStr;
  int correctPos, correctCol;

  // Read pattern string and feedback numbers
  if (!(iss >> patternStr >> correctPos >> correctCol)) {
    throw std::runtime_error(
        "Invalid format. Expected: 'pattern pos col' (e.g., 'rgbc 2 1')");
  }

  // Parse pattern string (no spaces between characters)
  std::array<uint8_t, MAX_PEGS> colors = {};
  uint8_t numPegs = 0;

  for (char c : patternStr) {
    if (numPegs >= MAX_PEGS) {
      break;
    }
    int colorIdx = config.charToColor(c);
    if (colorIdx < 0) {
      throw std::runtime_error("Invalid color character '" + std::string(1, c) +
                               "'. Available colors: " + config.colorChars);
    }
    colors[numPegs] = static_cast<uint8_t>(colorIdx);
    numPegs++;
  }

  if (numPegs != config.numPegs) {
    throw std::runtime_error("Pattern must have exactly " +
                             std::to_string(config.numPegs) + " colors");
  }

  // Create pattern using array constructor which automatically calls
  // computeColorCount()
  Pattern guess(colors, numPegs);

  // Validate feedback values
  if (correctPos < 0 || correctPos > static_cast<int>(config.numPegs) ||
      correctCol < 0 || correctCol > static_cast<int>(config.numPegs)) {
    throw std::runtime_error(
        "Feedback values must be between 0 and number of pegs");
  }

  Feedback fb;
  fb.guess = guess;
  fb.correctPosition = static_cast<uint8_t>(correctPos);
  fb.correctColor = static_cast<uint8_t>(correctCol);
  return fb;
}

// Helper: Check if a pattern matches feedback constraints
bool matchesFeedback(const Pattern &candidate, const Feedback &fb) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  const Pattern &guess = fb.guess;
  if (candidate.numPegs != guess.numPegs)
    return false;

  std::array<uint8_t, 256> candidateCount = candidate.colorCount;

  // Count correct positions and adjust counts
  int correctPositions = 0;
  for (uint8_t i = 0; i < candidate.numPegs; ++i) {
    if (candidate.colors[i] == guess.colors[i]) {
      correctPositions++;
      candidateCount[candidate.colors[i]]--;
    }
  }

  if (correctPositions != fb.correctPosition)
    return false;

  // Count correct colors in wrong positions
  int correctColors = 0;
  for (uint8_t i = 0; i < guess.numPegs; ++i) {
    // Skip positions that were already correct
    if (candidate.colors[i] == guess.colors[i])
      continue;

    if (candidateCount[guess.colors[i]] > 0) {
      correctColors++;
      candidateCount[guess.colors[i]]--;
    }
  }

  return correctColors == fb.correctColor;
}

// Generate feedback for a guess against a target pattern
Feedback generateFeedback(const Pattern &target, const Pattern &guess) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  Feedback fb;
  fb.guess = guess; // Store the guess in the feedback

  if (target.numPegs != guess.numPegs)
    return fb; // Invalid input

  std::array<uint8_t, 256> targetCount = target.colorCount;

  // First pass: count correct positions
  for (uint8_t i = 0; i < target.numPegs; ++i) {
    if (target.colors[i] == guess.colors[i]) {
      fb.correctPosition++;
      targetCount[target.colors[i]]--;
    }
  }

  // Second pass: count correct colors in wrong positions
  for (uint8_t i = 0; i < guess.numPegs; ++i) {
    // Skip positions that were already correct
    if (target.colors[i] == guess.colors[i])
      continue;

    if (targetCount[guess.colors[i]] > 0) {
      fb.correctColor++;
      targetCount[guess.colors[i]]--;
    }
  }

  return fb;
}

// Generate all possible patterns for the given configuration
std::vector<Pattern> generateAllPatterns(const Config &config) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  std::vector<Pattern> patterns;
  uint8_t numColors = config.numColors();

  // Pre-calculate total patterns and reserve space
  size_t totalPatterns;
  if (config.allowDuplicates) {
    // n^k combinations with repetition
    totalPatterns = 1;
    for (uint8_t i = 0; i < config.numPegs; ++i) {
      totalPatterns *= numColors;
    }
  } else {
    // P(n,k) = n!/(n-k)! permutations without repetition
    totalPatterns = 1;
    for (uint8_t i = 0; i < config.numPegs; ++i) {
      totalPatterns *= (numColors - i);
    }
  }
  patterns.reserve(totalPatterns);

  if (config.allowDuplicates) {
    // Generate all possible combinations with repetition
    std::array<uint8_t, MAX_PEGS> colors = {};
    uint8_t numPegs = config.numPegs;

    std::function<void(unsigned int)> generate = [&](unsigned int pos) {
      if (pos == config.numPegs) {
        // Create pattern using array constructor which automatically calls
        // computeColorCount()
        patterns.emplace_back(colors, numPegs);
        return;
      }

      for (unsigned int color = 0; color < numColors; ++color) {
        colors[pos] = static_cast<uint8_t>(color);
        generate(pos + 1);
      }
    };

    generate(0);
  } else {
    // Generate all possible permutations without repetition
    if (numColors < config.numPegs) {
      // Not enough colors for the number of pegs
      return patterns;
    }

    std::array<uint8_t, 256> availableColors; // Support up to 256 colors
    for (unsigned int i = 0; i < numColors; ++i) {
      availableColors[i] = static_cast<uint8_t>(i);
    }

    std::array<uint8_t, MAX_PEGS> colors = {};
    uint8_t numPegs = config.numPegs;
    std::vector<bool> used(numColors, false);

    std::function<void(unsigned int)> generate = [&](unsigned int pos) {
      if (pos == config.numPegs) {
        // Create pattern using array constructor which automatically calls
        // computeColorCount()
        patterns.emplace_back(colors, numPegs);
        return;
      }

      for (unsigned int i = 0; i < numColors; ++i) {
        if (!used[i]) {
          used[i] = true;
          colors[pos] = static_cast<uint8_t>(i);
          generate(pos + 1);
          used[i] = false;
        }
      }
    };

    generate(0);
  }

  return patterns;
}

// Mastermind solver traits
struct MastermindSolverTraits {
  using CandidateType = Pattern;
  using GuessType = Pattern;
  using FeedbackType = Feedback;
  using ConfigType = Config;
  using CalculatedGuessType = PatternGuess;
  using ResultType = Result;
  using CandidateSetType = Utils::SetCandidateSet<Pattern>;
};

// Mastermind-specific ENT solver implementation
class MastermindEntSolver
    : public Utils::AbstractEntSolverSameType<MastermindSolverTraits> {
public:
  MastermindEntSolver(const Config &cfg)
      : Utils::AbstractEntSolverSameType<MastermindSolverTraits>(cfg) {}

protected:
  bool matchesFeedback(const Pattern &candidate,
                       const Feedback &feedback) const override {
    return Mastermind::matchesFeedback(candidate, feedback);
  }

  Feedback generateFeedback(const Pattern &target,
                            const Pattern &guess) const override {
    return Mastermind::generateFeedback(target, guess);
  }

  PatternGuess createGuess(const Pattern &pattern, double ent, double wnt,
                           double probability) const override {
    PatternGuess guess;
    guess.pattern = pattern;
    guess.ent = ent;
    guess.wnt = wnt;
    guess.probability = probability;
    return guess;
  }

  Result createResult(const std::vector<PatternGuess> &guesses,
                      int totalPossible) const override {
    Result result;
    result.sortedGuesses = guesses;
    result.totalPossiblePatterns = totalPossible;
    result.searchDepth = this->activeDepth;
    return result;
  }

  double maxFeedbackGroups() const override {
    // Valid (exact, misplaced) pairs where exact + misplaced <= numPegs
    return static_cast<double>((config.numPegs + 1) * (config.numPegs + 2)) / 2.0;
  }

  double feedbackEfficiency() const override {
    return 0.95; // Highly independent peg options
  }
};

#include <atomic>

Result runMastermindSolver(const Config &config, std::atomic<bool> *cancel) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif

  // Generate all possible patterns for the given configuration
  std::vector<Pattern> allPatterns = Mastermind::generateAllPatterns(config);

  // Exclude already guessed patterns to speed up lookup
  std::unordered_set<Pattern> guessedPatterns;
  for (const auto &fb : config.feedbackHistory) {
    guessedPatterns.insert(fb.guess);
  }

  std::vector<Pattern> possiblePatterns;
  possiblePatterns.reserve(allPatterns.size());
  for (const auto &p : allPatterns) {
    if (guessedPatterns.count(p) == 0) {
      possiblePatterns.push_back(p);
    }
  }

  // Create CandidateSet from the filtered patterns
  Utils::VectorCandidateSet<Mastermind::Pattern> initialCandidates(possiblePatterns);

  // Use the specialized Mastermind ENT solver - returns Result directly!
  MastermindEntSolver solver(config);
  return solver.solve(possiblePatterns, initialCandidates, cancel);
}
} // namespace Mastermind
