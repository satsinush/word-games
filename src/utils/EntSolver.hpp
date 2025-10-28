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
 * - TCandidateType: The type representing a candidate solution (e.g., Word,
 * Pattern)
 * - TFeedbackType: The type representing feedback from a guess
 * - TConfigType: Configuration type for the solver
 * - TGuessType: The specific guess type to return (e.g., WordGuess,
 * PatternGuess)
 * - TResultType: The specific result type to return (e.g., Wordle::Result,
 * Mastermind::Result)
 */
template <typename TCandidateType, typename TFeedbackType, typename TConfigType,
          typename TGuessType, typename TResultType>
class AbstractEntSolver {
public:
  virtual ~AbstractEntSolver() = default;

protected:
  // Pure virtual methods to be implemented by derived classes
  virtual bool matchesFeedback(const TCandidateType &candidate,
                               const TFeedbackType &feedback) const = 0;
  virtual TFeedbackType generateFeedback(const TCandidateType &target,
                                         const TCandidateType &guess) const = 0;

  // Pure virtual methods for creating result objects directly
  virtual TGuessType createGuess(const TCandidateType &candidate, double ent,
                                 double probability) const = 0;
  virtual TResultType createResult(const std::vector<TGuessType> &guesses,
                                   int totalPossible) const = 0;

public:
  /**
   * Main solver function that returns the final result type directly
   */
  TResultType solve(const std::vector<TCandidateType> &allCandidates,
                    const std::vector<TCandidateType> &possibleCandidates,
                    const TConfigType &config,
                    std::atomic<bool> *cancel = nullptr) {
    // Store cancellation pointer so internal helpers can check it
    cancellationFlag = cancel;

    // Filter candidates based on feedback history
    std::vector<TCandidateType> filteredPossibleCandidates;

    // Create unordered_set using std::hash and std::equal_to (default template
    // specializations)
    std::unordered_set<TCandidateType> filteredPossibleCandidateSet;

    for (const auto &candidate : possibleCandidates) {
      bool matches = true;
      for (const auto &feedback : config.feedbackHistory) {
        if (!matchesFeedback(candidate, feedback)) {
          matches = false;
          break;
        }
      }
      if (matches) {
        filteredPossibleCandidates.push_back(candidate);
        filteredPossibleCandidateSet.insert(candidate);
      }
      if (cancellationFlag && cancellationFlag->load()) {
        // Clean up and return an empty result early on cancellation
        cancellationFlag = nullptr;
        return createResult(std::vector<TGuessType>{}, 0);
      }
    }

    std::vector<TGuessType> guesses;
    int totalPossible = static_cast<int>(filteredPossibleCandidates.size());

    // If maxDepth is 0, skip ENT calculation and just return filtered
    // candidates
    if (config.maxDepth == 0) {
      double possibleProb = filteredPossibleCandidates.empty()
                                ? 0.0
                                : 1.0 / filteredPossibleCandidates.size();
      for (const auto &candidate : allCandidates) {
        bool isPossible = filteredPossibleCandidateSet.find(candidate) !=
                          filteredPossibleCandidateSet.end();
        double probability = isPossible ? possibleProb : 0.0;
        TGuessType guess = createGuess(
            candidate,
            std::log2(static_cast<double>(filteredPossibleCandidates.size())),
            probability);
        guesses.push_back(guess);
      }

      std::sort(guesses.begin(), guesses.end());

      return createResult(guesses, totalPossible);
    }

    // Calculate Expected Number of Turns (ENT) for all candidates
    const double possibleProb = filteredPossibleCandidates.empty()
                                    ? 0.0
                                    : 1.0 / filteredPossibleCandidates.size();
    for (const auto &guessCandidate : allCandidates) {
      if (cancellationFlag && cancellationFlag->load()) {
        cancellationFlag = nullptr;
        return createResult(std::vector<TGuessType>{}, 0);
      }
      // Calculate probability (how likely this candidate is to be the answer)
      // O(1) lookup in unordered_set with proper hash and equality
      bool isPossible = filteredPossibleCandidateSet.find(guessCandidate) !=
                        filteredPossibleCandidateSet.end();

      double probability = isPossible ? possibleProb : 0.0;

      // If there is only one candidate left, the expected turns after guessing
      // the correct candidate is 0, otherwise it is 1
      double expectedTurns = isPossible ? 0.0 : 1.0;
      // If there are multiple possible candidates, calculate the expected turns
      // calculateExpectedTurns gets the ENT after this guess, assuming the
      // guess is not correct So we multiply by (1 - probability) to weight it
      // by the chance the guess is wrong
      if (filteredPossibleCandidates.size() > 1) {
        expectedTurns =
            (1 - probability) *
            calculateExpectedTurns(guessCandidate, filteredPossibleCandidates,
                                   allCandidates, config.maxDepth);
      }

      TGuessType guess =
          createGuess(guessCandidate, expectedTurns, probability);
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

private:
  // Cancellation pointer set during solve(); helpers check this and return
  // early when set.
  std::atomic<bool> *cancellationFlag = nullptr;
  // Helper struct for grouping candidates with total score
  struct FeedbackGroup {
    std::vector<TCandidateType> candidates;
    double totalScore = 0.0;
  };

  /**
   * Single-value minimax helper that finds the minimum expected uncertainty
   * after 'depth' more moves using optimal strategy.
   * This returns ONLY the final minimum value, not the full path.
   */
  double findMinExpectedTurns(
      const std::vector<TCandidateType> &currentCandidates,
      const std::vector<TCandidateType> &allCandidates,
      const int maxDepth) { // Depth limit to prevent infinite recursion
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    // Base Case: If only one candidate left, it has already been found
    if (currentCandidates.size() <= 1)
      return 0.0;

    // Depth limit reached - return pessimistic estimate
    if (maxDepth <= 0)
      return std::log2(static_cast<double>(
          currentCandidates.size())); // Worst case: binary search depth

    double minExpectedTurns = std::numeric_limits<double>::max();

    // ITERATE OVER ALL POSSIBLE NEXT GUESSES to find the optimal one
    for (const auto &nextGuess : allCandidates) {
      if (cancellationFlag && cancellationFlag->load())
        return std::numeric_limits<double>::infinity();
      double expectedTurns = calculateExpectedTurns(
          nextGuess, currentCandidates, allCandidates, maxDepth);

      // Minimax: Find the best next guess (minimize expected turns)
      minExpectedTurns = std::min(minExpectedTurns, expectedTurns);
    }

    return minExpectedTurns;
  }

  /**
   * Calculates the Expected Number of Turns (ENT) that will need to be taken
   * after making a specific guess.
   * If the guess is the only candidate left, ENT is 0 (solved).
   * If the guess will leave only one candidate left, ENT is 1 (next turn
   * solves).
   */
  double
  calculateExpectedTurns(const TCandidateType &guessCandidate,
                         const std::vector<TCandidateType> &currentCandidates,
                         const std::vector<TCandidateType> &allCandidates,
                         const int maxDepth) {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif

    // Calculate total score for normalization
    double totalScore = 0.0;
    for (const auto &target : currentCandidates) {
      totalScore += target.score;
    }

    // Group candidates by feedback pattern for this guess, with weighted scores
    std::unordered_map<TFeedbackType, FeedbackGroup> feedbackGroups;
    feedbackGroups.reserve(currentCandidates.size());

    for (const auto &target : currentCandidates) {
      if (cancellationFlag && cancellationFlag->load())
        return std::numeric_limits<double>::infinity();
      TFeedbackType feedback = generateFeedback(target, guessCandidate);
      feedbackGroups[feedback].candidates.push_back(target);
      feedbackGroups[feedback].totalScore += target.score;
    }

    // Calculate Expected Number of Turns for this guess using weighted
    // probabilities
    double expectedTurns = 0.0;

    for (const auto &group : feedbackGroups) {
      double prob = group.second.totalScore / totalScore;

      if (group.second.candidates.size() == 1) {
        // This feedback group leads to a solution in 1 more turn
        expectedTurns += prob * 1.0;
      } else {
        // This feedback group requires optimal play on the subgroup
        double optimalSubTurns = findMinExpectedTurns(
            group.second.candidates, allCandidates, maxDepth - 1);
        expectedTurns += prob * (1.0 + optimalSubTurns);
      }
    }

    return expectedTurns;
  }
};
} // namespace Utils