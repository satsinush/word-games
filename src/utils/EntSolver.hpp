#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

namespace Utils {
/**
 * Abstract base class for ENT-based puzzle solvers.
 * Implements the generic Expected Number of Turns calculation algorithm.
 * Template parameters:
 * - TSolutionType: The type representing a candidate solution (e.g., Word,
 * Pattern)
 * - TGuessInputType: The type representing a guess input (e.g., char for
 * Hangman, Word for Wordle). Defaults to TSolutionType for games where guesses
 * and solutions are the same type.
 * - TFeedbackType: The type representing feedback from a guess
 * - TConfigType: Configuration type for the solver
 * - TGuessType: The specific guess type to return (e.g., WordGuess,
 * PatternGuess, LetterGuess)
 * - TResultType: The specific result type to return (e.g., Wordle::Result,
 * Mastermind::Result)
 */
template <typename TSolutionType, typename TGuessInputType,
          typename TFeedbackType, typename TConfigType, typename TGuessType,
          typename TResultType>
class AbstractEntSolver {
public:
  virtual ~AbstractEntSolver() = default;

  AbstractEntSolver(const TConfigType &config) : config(config) {}

protected:
  // Pure virtual methods to be implemented by derived classes
  virtual bool matchesFeedback(const TSolutionType &candidate,
                               const TFeedbackType &feedback) const = 0;
  virtual TFeedbackType
  generateFeedback(const TSolutionType &target,
                   const TGuessInputType &guess) const = 0;

  // Pure virtual methods for creating result objects directly
  virtual TGuessType createGuess(const TGuessInputType &guess, double ent,
                                 double probability) const = 0;
  virtual TResultType createResult(const std::vector<TGuessType> &guesses,
                                   int totalPossible) const = 0;

  // Get the score for a guess input (used for probability calculations)
  // Default implementation returns 1.0 for uniform weighting
  virtual double getGuessScore(const TGuessInputType &guess) const {
    (void)guess; // Suppress unused parameter warning
    return 1.0;
  }

  virtual double worstCaseExpectedTurns(size_t numCandidates) const {
    return std::log2(static_cast<double>(numCandidates));
  }

  // Check if the current set of solutions represents a "solved" state.
  // Default: solved when 1 or fewer solutions remain.
  // Override for games like multi-word Hangman where "solved" means
  // one solution per word slot (e.g., 5 solutions for 5 words).
  virtual bool
  isSolvedState(const std::vector<TSolutionType> &currentSolutions) const {
    return currentSolutions.size() <= 1;
  }

public:
  /**
   * Main solver function that returns the final result type directly
   * @param allGuesses All possible guess inputs to evaluate
   * @param possibleSolutions All possible solutions that could be the answer
   * @param cancel Optional atomic flag to cancel the operation
   */
  TResultType solve(const std::vector<TGuessInputType> &allGuesses,
                    const std::vector<TSolutionType> &possibleSolutions,
                    std::atomic<bool> *cancel = nullptr) {
    // Store cancellation pointer so internal helpers can check it
    cancellationFlag = cancel;

    // Filter solutions based on feedback history using pointers for
    // performance
    std::vector<const TSolutionType *> filteredPossibleSolutionPtrs;
    filteredPossibleSolutionPtrs.reserve(possibleSolutions.size());

    // Create unordered_set using std::hash and std::equal_to (default template
    // specializations) - still need objects for hash/equality
    std::unordered_set<TSolutionType> filteredPossibleSolutionSet;

    for (const auto &solution : possibleSolutions) {
      bool matches = true;
      for (const auto &feedback : config.feedbackHistory) {
        if (!matchesFeedback(solution, feedback)) {
          matches = false;
          break;
        }
      }
      if (matches) {
        filteredPossibleSolutionPtrs.push_back(&solution);
        filteredPossibleSolutionSet.insert(solution);
      }
      if (cancellationFlag && cancellationFlag->load()) {
        // Clean up and return an empty result early on cancellation
        cancellationFlag = nullptr;
        return createResult(std::vector<TGuessType>{}, 0);
      }
    }

    // Convert pointers back to objects only when needed for calculations
    std::vector<TSolutionType> filteredPossibleSolutions;
    filteredPossibleSolutions.reserve(filteredPossibleSolutionPtrs.size());
    for (const auto *solutionPtr : filteredPossibleSolutionPtrs) {
      filteredPossibleSolutions.push_back(*solutionPtr);
    }

    std::vector<TGuessType> guesses;
    int totalPossible = static_cast<int>(filteredPossibleSolutions.size());

    // If maxDepth is 0, skip ENT calculation and just return filtered
    // solutions
    if (config.maxDepth == 0) {
      double possibleProb = filteredPossibleSolutions.empty()
                                ? 0.0
                                : 1.0 / filteredPossibleSolutions.size();
      for (const auto &guess : allGuesses) {
        // For games where guess type differs from solution type,
        // probability is calculated differently
        double probability = calculateGuessProbability(
            guess, filteredPossibleSolutionSet, possibleProb);
        TGuessType guessResult = createGuess(
            guess, worstCaseExpectedTurns(filteredPossibleSolutions.size()),
            probability);
        guesses.push_back(guessResult);
      }

      std::sort(guesses.begin(), guesses.end());

      return createResult(guesses, totalPossible);
    }

    // Calculate Expected Number of Turns (ENT) for all guesses
    const double possibleProb = filteredPossibleSolutions.empty()
                                    ? 0.0
                                    : 1.0 / filteredPossibleSolutions.size();
    for (const auto &guessInput : allGuesses) {
      if (cancellationFlag && cancellationFlag->load()) {
        cancellationFlag = nullptr;
        return createResult(std::vector<TGuessType>{}, 0);
      }
      // Calculate probability for this guess
      double probability = calculateGuessProbability(
          guessInput, filteredPossibleSolutionSet, possibleProb);

      double expectedTurns =
          (1 - probability) *
          calculateExpectedTurns(guessInput, filteredPossibleSolutions,
                                 allGuesses, config.maxDepth);

      TGuessType guess = createGuess(guessInput, expectedTurns, probability);
      guesses.push_back(guess);
    }

    // Sort by Expected Number of Turns (lowest first - best guesses minimize
    // turns) Secondary sort by probability (highest first - prefer possible
    // answers as tiebreaker) Assumes TGuessType has operator< that sorts by ENT
    // ascending, then probability descending
    std::sort(guesses.begin(), guesses.end());

    cancellationFlag = nullptr;
    return createResult(guesses, totalPossible);
  }

protected:
  TConfigType config;

  // Virtual method to calculate probability for a guess
  // Default implementation: if guess type equals solution type, check if guess
  // is in possible solutions For games where guess type differs from solution
  // type, override this method
  virtual double calculateGuessProbability(
      const TGuessInputType &guess,
      const std::unordered_set<TSolutionType> &possibleSolutions,
      double possibleProb) const {
    // Default implementation for when TGuessInputType == TSolutionType
    // This uses SFINAE-like behavior through virtual dispatch
    (void)guess;
    (void)possibleSolutions;
    (void)possibleProb;
    return 0.0; // Override in derived classes
  }

private:
  // Cancellation pointer set during solve(); helpers check this and return
  // early when set.
  std::atomic<bool> *cancellationFlag = nullptr;

  // Helper struct for grouping solutions with total score
  struct FeedbackGroup {
    std::vector<TSolutionType> solutions;
    double totalScore = 0.0;
  };

  /**
   * Single-value minimax helper that finds the minimum expected uncertainty
   * after 'depth' more moves using optimal strategy.
   * This returns ONLY the final minimum value, not the full path.
   */
  double findMinExpectedTurns(
      const std::vector<TSolutionType> &currentSolutions,
      const std::vector<TGuessInputType> &allGuesses,
      const int maxDepth) { // Depth limit to prevent infinite recursion
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    // Base Case: Check if we've reached a solved state
    if (isSolvedState(currentSolutions))
      return 0.0;

    // Depth limit reached - return pessimistic estimate
    if (maxDepth <= 0)
      return worstCaseExpectedTurns(currentSolutions.size());

    double minExpectedTurns = std::numeric_limits<double>::max();

    // ITERATE OVER ALL POSSIBLE NEXT GUESSES to find the optimal one
    for (const auto &nextGuess : allGuesses) {
      if (cancellationFlag && cancellationFlag->load())
        return std::numeric_limits<double>::infinity();
      double expectedTurns = calculateExpectedTurns(nextGuess, currentSolutions,
                                                    allGuesses, maxDepth);

      // Minimax: Find the best next guess (minimize expected turns)
      minExpectedTurns = std::min(minExpectedTurns, expectedTurns);
    }

    return minExpectedTurns;
  }

  /**
   * Calculates the Expected Number of Turns (ENT) that will need to be taken
   * after making a specific guess.
   * If the guess correctly identifies the solution, ENT is 0 (solved).
   * If the guess will leave only one solution left, ENT is 1 (next turn
   * solves).
   */
  double
  calculateExpectedTurns(const TGuessInputType &guessInput,
                         const std::vector<TSolutionType> &currentSolutions,
                         const std::vector<TGuessInputType> &allGuesses,
                         const int maxDepth) {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif

    // Base Cases
    if (currentSolutions.empty()) {
      // No solutions left (should not happen in normal play)
      return 0.0;
    } else if (isSolvedState(currentSolutions)) {
      // Already in solved state - check if guess directly solves
      // For single-solution games, this checks if guess matches the solution
      if (currentSolutions.size() == 1) {
        return isGuessSolution(guessInput, currentSolutions[0]) ? 0.0 : 1.0;
      }
      // For multi-slot games (e.g., hangman), already solved
      return 0.0;
    }

    // Calculate total score for normalization
    double totalScore = 0.0;
    for (const auto &target : currentSolutions) {
      totalScore += getSolutionScore(target);
    }

    // Group solutions by feedback pattern for this guess, with weighted scores
    std::unordered_map<TFeedbackType, FeedbackGroup> feedbackGroups;
    feedbackGroups.reserve(currentSolutions.size());

    for (const auto &target : currentSolutions) {
      if (cancellationFlag && cancellationFlag->load())
        return std::numeric_limits<double>::infinity();
      TFeedbackType feedback = generateFeedback(target, guessInput);
      feedbackGroups[feedback].solutions.push_back(target);
      feedbackGroups[feedback].totalScore += getSolutionScore(target);
    }

    // Calculate Expected Number of Turns for this guess using weighted
    // probabilities
    double expectedTurns = 0.0;

    for (const auto &group : feedbackGroups) {
      double prob = group.second.totalScore / totalScore;

      double optimalSubTurns = findMinExpectedTurns(group.second.solutions,
                                                    allGuesses, maxDepth - 1);
      expectedTurns += prob * (1.0 + optimalSubTurns);
    }

    return expectedTurns;
  }

  // Check if a guess is the solution (for games where guess type differs from
  // solution type) Default: returns false, override for specific comparison
  // logic
  virtual bool isGuessSolution(const TGuessInputType &guess,
                               const TSolutionType &solution) const {
    (void)guess;
    (void)solution;
    return false; // Override in derived classes
  }

  // Get the score for a solution (used for probability weighting)
  // Default implementation returns 1.0 for uniform weighting
  virtual double getSolutionScore(const TSolutionType &solution) const {
    (void)solution;
    return 1.0;
  }
};

/**
 * Convenience alias for games where guess type equals solution type (e.g.,
 * Wordle, Mastermind)
 */
template <typename TCandidateType, typename TFeedbackType, typename TConfigType,
          typename TGuessType, typename TResultType>
class AbstractEntSolverSameType
    : public AbstractEntSolver<TCandidateType, TCandidateType, TFeedbackType,
                               TConfigType, TGuessType, TResultType> {
public:
  using Base = AbstractEntSolver<TCandidateType, TCandidateType, TFeedbackType,
                                 TConfigType, TGuessType, TResultType>;
  using Base::Base;

protected:
  double calculateGuessProbability(
      const TCandidateType &guess,
      const std::unordered_set<TCandidateType> &possibleSolutions,
      double possibleProb) const override {
    bool isPossible = possibleSolutions.find(guess) != possibleSolutions.end();
    return isPossible ? possibleProb : 0.0;
  }

  bool isGuessSolution(const TCandidateType &guess,
                       const TCandidateType &solution) const override {
    return guess == solution;
  }

  double getSolutionScore(const TCandidateType &solution) const override {
    return solution.score;
  }
};
} // namespace Utils