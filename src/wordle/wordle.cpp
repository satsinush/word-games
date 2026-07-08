#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "utils/EntSolver.hpp"
#include "wordle/wordle.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

namespace Wordle {
Feedback parseFeedback(const std::string &input) {
  Feedback fb;
  std::string word;
  std::string colors;
  size_t space = input.find(' ');
  if (space == std::string::npos)
    throw std::runtime_error("Invalid feedback format");
  word = input.substr(0, space);
  colors = input.substr(space + 1);
  if (word.size() != colors.size())
    throw std::runtime_error("Word and colors must have same length");
  if (word.size() < 1 || word.size() > 16)
    throw std::runtime_error("Word length must be between 1 and 16");
  fb.word = Utils::trimToLower(word);
  if (word.size() > 32)
    throw std::runtime_error("Word length must be at most 32");
  for (size_t i = 0; i < word.size(); ++i) {
    if (colors[i] < '0' || colors[i] > '2')
      throw std::runtime_error("Invalid color digit");
    int color = colors[i] - '0';
    if (color == 0)
      fb.setGrey(i);
    else if (color == 1)
      fb.setYellow(i);
    else
      fb.setGreen(i);
  }
  return fb;
}

// Helper: Check if a word matches all feedback constraints
bool matchesFeedback(const Utils::Word &candidate, const Feedback &fb) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  const std::string &guess = fb.word;
  size_t wordLen = guess.size();

  // Words must be same length to match
  if (candidate.wordString.size() != wordLen) {
    return false;
  }

  // Use pre-calculated letter count from candidate word
  std::array<uint8_t, 26> candidateLetterCount = candidate.letterCount;

  // Check greens first, and adjust counts
  for (size_t i = 0; i < wordLen; ++i) {
    if (fb.colors[i * 2 + 1]) // Green
    {                         // Green
      if (candidate.wordString[i] != guess[i]) {
        return false; // Must match green letters
      }
      candidateLetterCount[candidate.wordString[i] - 'a']--;
    }
  }

  for (size_t i = 0; i < wordLen; ++i) {
    char guessChar = guess[i];
    if (fb.colors[i * 2 + 1]) { // Green (already checked)
      continue;
    } else if (fb.colors[i * 2]) { // Yellow
      if (candidate.wordString[i] == guessChar) {
        return false; // Yellow letter cannot be in the same spot
      }
      if (candidateLetterCount[guessChar - 'a'] <= 0) {
        return false; // Must contain the yellow letter elsewhere
      }
      candidateLetterCount[guessChar - 'a']--;
    } else { // Grey
      if (candidateLetterCount[guessChar - 'a'] > 0) {
        return false; // Candidate has a letter that was marked grey
      }
    }
  }

  return true;
}

// Generate feedback for a guess against a target word
Feedback generateFeedback(const Utils::Word &target, const std::string &guess) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  Feedback fb;
  fb.word = guess;

  size_t wordLen = guess.size();
  if (target.wordString.size() != wordLen) {
    // Mismatched lengths - return all grey
    return fb;
  }

  // The bitset is already initialized to all zeros (all grey)
  std::array<uint8_t, 26> letterCount = target.letterCount;

  // First pass: Mark greens. A green is represented by setting both bits for a
  // position to 1. Example for position i: bit (i*2) = 1, bit (i*2 + 1) = 1.
  for (size_t i = 0; i < wordLen; ++i) {
    if (target.wordString[i] == guess[i]) {
      // Direct manipulation: Set the 'green' bit and the 'yellow' bit.
      fb.colors[i * 2 + 1] = 1; // This bit signals "correct position" (Green).
      fb.colors[i * 2] = 1;     // This bit signals "in word" (Green or Yellow).

      letterCount[target.wordString[i] - 'a']--;
    }
  }

  // Second pass: Mark yellows. A yellow is when the "in word" bit is 1 but
  // "correct position" is 0.
  for (size_t i = 0; i < wordLen; ++i) {
    if (!fb.colors[i * 2 + 1]) // If it's NOT green...
    {
      int guessCharIndex = guess[i] - 'a';
      if (letterCount[guessCharIndex] > 0) {
        fb.colors[i * 2] = 1;
        letterCount[guessCharIndex]--;
      }
    }
  }

  return fb;
}

std::vector<Utils::Word> filterWords(const std::vector<Utils::Word> &words,
                                     const std::vector<Feedback> &feedbacks) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  std::vector<Utils::Word> filtered;
  for (const auto &w : words) {
    bool ok = true;
    for (const auto &fb : feedbacks) {
      if (!matchesFeedback(w, fb)) {
        ok = false;
        break;
      }
    }
    if (ok)
      filtered.push_back(w);
  }
  return filtered;
}

std::vector<Utils::Word>
runWordleSolver(const std::vector<Utils::Word> &words,
                const std::vector<Feedback> &feedbacks) {
  return filterWords(words, feedbacks);
}

// Wordle solver traits - bundles all types for the ENT solver
struct WordleSolverTraits {
  using CandidateType = Utils::Word;
  using GuessType = Utils::Word;
  using FeedbackType = Feedback;
  using ConfigType = Config;
  using CalculatedGuessType = WordGuess;
  using ResultType = Result;
  using CandidateSetType = Utils::SetCandidateSet<Utils::Word>;
};

// Wordle-specific ENT solver implementation
class WordleEntSolver : public Utils::AbstractEntSolverSameType<WordleSolverTraits> {
public:
  WordleEntSolver(const Config &cfg)
      : Utils::AbstractEntSolverSameType<WordleSolverTraits>(cfg) {}

protected:
  bool matchesFeedback(const Utils::Word &candidate,
                       const Feedback &feedback) const override {
    return Wordle::matchesFeedback(candidate, feedback);
  }

  Feedback generateFeedback(const Utils::Word &target,
                            const Utils::Word &guess) const override {
    return Wordle::generateFeedback(target, guess.wordString);
  }

  WordGuess createGuess(const Utils::Word &word, double ent, double probability) const override {
    WordGuess guess;
    guess.word = word;
    guess.ent = ent;
    guess.probability = probability;
    return guess;
  }

  Result createResult(const std::vector<WordGuess> &guesses,
                      int totalPossible) const override {
    Result result;
    result.sortedGuesses = guesses;
    result.totalPossibleWords = totalPossible;
    return result;
  }

  double worstCaseExpectedTurns(size_t numCandidates) const override {
    if (numCandidates <= 1) return 0.0;
    double base = std::max(2.0, static_cast<double>(config.wordLength));
    return std::log(static_cast<double>(numCandidates)) / std::log(base);
  }
};

Result runWordleSolver(const Config &config, std::atomic<bool> *cancel) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  std::vector<Utils::Word> allWords = Utils::loadWords();

  // Filter words to only words of the specified length
  std::vector<Utils::Word> possibleWords;
  for (const auto &word : allWords) {
    bool exclude = config.excludeUncommonWords && (!word.is_scrabble);
    if (word.wordString.length() == config.wordLength && !exclude) {
      possibleWords.push_back(word);
    }
  }

  // Create CandidateSet from the filtered words
  Utils::VectorCandidateSet<Utils::Word> initialCandidates(possibleWords);

  // Use the specialized Wordle ENT solver - returns Result directly!
  WordleEntSolver solver(config);
  return solver.solve(possibleWords, initialCandidates, cancel);
}
} // namespace Wordle