#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
                    const std::vector<TFeedbackType> &feedbackHistory,
                    const TConfigType &config) {
    // Filter candidates based on feedback history
    std::vector<TCandidateType> possibleCandidates;

    // Create unordered_set using std::hash and std::equal_to (default template
    // specializations)
    std::unordered_set<TCandidateType> possibleCandidateSet;

    for (const auto &candidate : allCandidates) {
      bool matches = true;
      for (const auto &feedback : feedbackHistory) {
        if (!matchesFeedback(candidate, feedback)) {
          matches = false;
          break;
        }
      }
      if (matches) {
        possibleCandidates.push_back(candidate);
        possibleCandidateSet.insert(candidate);
      }
    }

    std::vector<TGuessType> guesses;
    int totalPossible = static_cast<int>(possibleCandidates.size());

    // If maxDepth is 0, skip ENT calculation and just return filtered
    // candidates
    if (config.maxDepth == 0) {
      for (const auto &candidate : possibleCandidates) {
        double probability =
            possibleCandidates.empty() ? 0.0 : 1.0 / possibleCandidates.size();
        TGuessType guess = createGuess(candidate, 0.0, probability);
        guesses.push_back(guess);
      }
      return createResult(guesses, totalPossible);
    }

    // Calculate Expected Number of Turns (ENT) for all candidates
    for (const auto &guessCandidate : allCandidates) {
      // Calculate probability (how likely this candidate is to be the answer)
      // O(1) lookup in unordered_set with proper hash and equality
      bool isPossible = possibleCandidateSet.find(guessCandidate) !=
                        possibleCandidateSet.end();

      double probability = isPossible ? (possibleCandidates.empty()
                                             ? 0.0
                                             : 1.0 / possibleCandidates.size())
                                      : 0.0;

      // Calculate Expected Number of Turns (ENT) - primary sorting metric
      double expectedTurns = calculateExpectedTurns(
          guessCandidate, possibleCandidates, config.maxDepth);

      TGuessType guess =
          createGuess(guessCandidate, expectedTurns, probability);
      guesses.push_back(guess);
    }

    // Sort by Expected Number of Turns (lowest first - best guesses minimize
    // turns) Secondary sort by probability (highest first - prefer possible
    // answers as tiebreaker) Assumes TGuessType has operator< that sorts by ENT
    // ascending, then probability descending
    std::sort(guesses.begin(), guesses.end());

    return createResult(guesses, totalPossible);
  }

private:
  /**
   * Single-value minimax helper that finds the minimum expected uncertainty
   * after 'depth' more moves using optimal strategy.
   * This returns ONLY the final minimum value, not the full path.
   */
  double findMinExpectedTurns(
      const std::vector<TCandidateType> &currentCandidates,
      const std::vector<TCandidateType> &allCandidates,
      const int maxDepth) { // Depth limit to prevent infinite recursion

    // Base Case: If only one candidate left, it has already been found
    if (currentCandidates.size() <= 1)
      return 0.0;

    // Depth limit reached - return pessimistic estimate
    if (maxDepth <= 0)
      return std::log2(static_cast<double>(
          currentCandidates.size())); // Worst case: binary search depth

    double minExpectedTurns = std::numeric_limits<double>::max();

    // ITERATE OVER ALL POSSIBLE NEXT GUESSES
    for (const auto &nextGuess : allCandidates) {
      // Group current candidates based on this nextGuess
      std::unordered_map<TFeedbackType, std::vector<TCandidateType>>
          feedbackGroups;
      feedbackGroups.reserve(currentCandidates.size());

      for (const auto &target : currentCandidates) {
        TFeedbackType feedback = generateFeedback(target, nextGuess);
        feedbackGroups[feedback].push_back(target);
      }

      // Calculate Expected Number of Turns for this guess
      double expectedTurns = 0.0;
      double total = static_cast<double>(currentCandidates.size());

      for (const auto &group : feedbackGroups) {
        double prob = static_cast<double>(group.second.size()) / total;

        if (group.second.size() == 1) {
          // This feedback group leads to a solution in 1 more turn
          expectedTurns += prob * 1.0;
        } else {
          // This feedback group requires optimal play on the subgroup
          double optimalSubTurns =
              findMinExpectedTurns(group.second, allCandidates, maxDepth - 1);
          expectedTurns +=
              prob *
              (1.0 +
               optimalSubTurns); // 1 turn for this guess + optimal sub-turns
        }
      }

      // Minimax: Find the best next guess (minimize expected turns)
      minExpectedTurns = std::min(minExpectedTurns, expectedTurns);
    }

    return minExpectedTurns;
  }

  /**
   * Calculate Expected Number of Turns (ENT) for a specific guess.
   * Returns the expected number of turns needed to solve the puzzle
   * if this guess is chosen as the next move.
   */
  double
  calculateExpectedTurns(const TCandidateType &guessCandidate,
                         const std::vector<TCandidateType> &possibleCandidates,
                         const int maxDepth) {

    if (possibleCandidates.empty())
      return 1.0; // If no candidates, assume 1 turn (shouldn't happen)

    // Group candidates by feedback pattern for this guess
    std::unordered_map<TFeedbackType, std::vector<TCandidateType>>
        feedbackGroups;
    feedbackGroups.reserve(possibleCandidates.size());
    for (const auto &target : possibleCandidates) {
      TFeedbackType feedback = generateFeedback(target, guessCandidate);
      feedbackGroups[feedback].push_back(target);
    }

    // Calculate Expected Number of Turns for this guess
    double expectedTurns = 0.0;
    double total = static_cast<double>(possibleCandidates.size());

    for (const auto &group : feedbackGroups) {
      double prob = static_cast<double>(group.second.size()) / total;

      if (group.second.size() == 1) {
        // We know the optimal turns remaining is 0, so avoid the function call
        // entirely
        expectedTurns += prob; // * 1.0 for this turn
      } else {
        // This feedback group requires optimal play on the subgroup
        double optimalSubTurns = findMinExpectedTurns(
            group.second, possibleCandidates, maxDepth - 1);
        expectedTurns += prob * (1.0 + optimalSubTurns);
      }
    }

    return expectedTurns;
  }
};
} // namespace Utils