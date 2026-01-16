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
 */
template <typename TCandidateType, typename TGuessType, typename TFeedbackType,
          typename TConfigType, typename TCalculatedGuess, typename TResultType,
          typename TCandidateSet>
class AbstractEntSolver {
public:
  virtual ~AbstractEntSolver() = default;

  AbstractEntSolver(const TConfigType &config) : config(config) {}

protected:
  // Pure virtual methods to be implemented by derived classes
  virtual bool matchesFeedback(const TCandidateType &candidate,
                               const TFeedbackType &feedback) const = 0;
  virtual TFeedbackType generateFeedback(const TCandidateType &target,
                                         const TGuessType &guess) const = 0;

  // Pure virtual methods for creating result objects directly
  virtual TCalculatedGuess createGuess(const TGuessType &guess, double ent) const = 0;
  virtual TResultType createResult(const std::vector<TCalculatedGuess> &guesses,
                                   int totalPossible) const = 0;

  // Get the score for a guess input (used for probability calculations)
  // Default implementation returns 1.0 for uniform weighting
  virtual double getGuessScore(const TGuessType &guess) const {
    (void)guess; 
    return 1.0;
  }

  // Create next candidate set from current set based on feedback
  virtual TCandidateSet filterCandidates(const TCandidateSet &candidates,
                                         const TGuessType &guess,
                                         const TFeedbackType &feedback) const {
    return candidates.filter(guess, feedback,
                                    [this](const TCandidateType &c,
                                           const TFeedbackType &f) {
                                      return this->matchesFeedback(c, f);
                                    });
  }

  virtual double worstCaseExpectedTurns(size_t numCandidates) const {
    return std::log2(static_cast<double>(numCandidates));
  }

public:
  /**
   * Main solver function that returns the final result type directly
   * @param allGuesses All possible guess inputs to evaluate
   * @param initialCandidates The starting set of candidates
   * @param cancel Optional atomic flag to cancel the operation
   */
  TResultType solve(const std::vector<TGuessType> &allGuesses,
                    const TCandidateSet &initialCandidates,
                    std::atomic<bool> *cancel = nullptr) {
    cancellationFlag = cancel;

    TCandidateSet filteredCandidates = initialCandidates;
    
    // Apply existing feedback history
    for (const auto &feedback : config.feedbackHistory) {
         filteredCandidates = filteredCandidates.filter([this, &feedback](const TCandidateType& c) {
             return this->matchesFeedback(c, feedback);
         });
         
        if (cancellationFlag && cancellationFlag->load()) {
            cancellationFlag = nullptr;
            return createResult(std::vector<TCalculatedGuess>{}, 0);
        }
    }

    std::vector<TCalculatedGuess> guesses;
    int totalPossible = static_cast<int>(filteredCandidates.size());

    // If maxDepth is 0, skip ENT calculation and just return filtered solutions
    if (config.maxDepth == 0) {
      for (const auto &guess : allGuesses) {
        // Just return worst case (or 0?) since we aren't calculating
        TCalculatedGuess guessResult = createGuess(
            guess, worstCaseExpectedTurns(filteredCandidates.size()));
        guesses.push_back(guessResult);
      }
      // Sort logic might depend on ENT, but here it's uniform.
      // Derived classes usually sort by probability too? 
      // User said "remove calculate guess probability since it doesn't matter".
      // So sorting is likely just by ENT (which is equal) or stable sort.

      std::sort(guesses.begin(), guesses.end());

      return createResult(guesses, totalPossible);
    }

    // Calculate Expected Number of Turns (ENT) for all guesses
    for (const auto &guessInput : allGuesses) {
      if (cancellationFlag && cancellationFlag->load()) {
        cancellationFlag = nullptr;
        return createResult(std::vector<TCalculatedGuess>{}, 0);
      }
      
      double expectedTurns = calculateExpectedTurns(guessInput, filteredCandidates,
                                                    allGuesses, config.maxDepth);

      TCalculatedGuess guess = createGuess(guessInput, expectedTurns);
      guesses.push_back(guess);
    }

    std::sort(guesses.begin(), guesses.end());

    cancellationFlag = nullptr;
    return createResult(guesses, totalPossible);
  }

protected:
  TConfigType config;

private:
  // Cancellation pointer set during solve(); helpers check this and return early
  std::atomic<bool> *cancellationFlag = nullptr;

  /**
   * Single-value minimax helper that finds the minimum expected uncertainty
   */
  double findMinExpectedTurns(
      const TCandidateSet &candidates,
      const std::vector<TGuessType> &allGuesses,
      const int maxDepth) {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    if (candidates.size() <= 1)
      return 0.0;

    if (maxDepth <= 0)
      return worstCaseExpectedTurns(candidates.size());

    double minExpectedTurns = std::numeric_limits<double>::max();

    for (const auto &nextGuess : allGuesses) {
      if (cancellationFlag && cancellationFlag->load())
        return std::numeric_limits<double>::infinity();
      double expectedTurns = calculateExpectedTurns(nextGuess, candidates,
                                                    allGuesses, maxDepth);

      minExpectedTurns = std::min(minExpectedTurns, expectedTurns);
    }

    return minExpectedTurns;
  }

  /**
   * Calculates the Expected Number of Turns (ENT) that will need to be taken
   * after making a specific guess.
   */
  double
  calculateExpectedTurns(const TGuessType &guessInput,
                         const TCandidateSet &candidates,
                         const std::vector<TGuessType> &allGuesses,
                         const int maxDepth) {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif

    if (candidates.size() <= 1) {
        // If solved (0 or 1 candidate), 0 more turns needed.
        return 0.0; 
    }

    double totalScore = candidates.totalScore();
    double expectedRemainingTurns = 0.0;
    
    // ENT = 1 (this guess) + Expected Turns for Subproblems
    // Sum( P(outcome) * MinTurns(outcome) )
    
    candidates.visitFeedbackGroups(
        guessInput,
        [this, &expectedRemainingTurns, totalScore, &allGuesses, maxDepth](const TFeedbackType&, const TCandidateSet& subset, double subsetScore) {
             
             double prob = subsetScore / totalScore;
             if (prob > 0) {
                 double optimalSubTurns = findMinExpectedTurns(subset, allGuesses, maxDepth - 1);
                 expectedRemainingTurns += prob * optimalSubTurns;
             }
        },
        [this](const TCandidateType& c, const TGuessType& g) {
            return this->generateFeedback(c, g);
        }
    );

    return 1.0 + expectedRemainingTurns;
  }
};


/**
 * A standard implementation of TCandidateSet that wraps a std::vector.
 * Used for games where the solution space is small enough to enumerate fully
 * (e.g., Wordle, Mastermind).
 */
template <typename TCandidateType> class ConcreteCandidateSet {
public:
  using Container = std::vector<TCandidateType>;

  ConcreteCandidateSet() = default;
  explicit ConcreteCandidateSet(const Container &candidates)
      : candidates(candidates) {
    recalculateTotalScore();
  }
  explicit ConcreteCandidateSet(Container &&candidates)
      : candidates(std::move(candidates)) {
    recalculateTotalScore();
  }

  size_t size() const { return candidates.size(); }
  bool empty() const { return candidates.empty(); }
  double totalScore() const { return cachedTotalScore; }

  auto begin() const { return candidates.begin(); }
  auto end() const { return candidates.end(); }

  bool contains(const TCandidateType &candidate) const {
    for (const auto &c : candidates) {
      if (c == candidate)
        return true;
    }
    return false;
  }

  template <typename TGuessType, typename TFeedbackType, typename Predicate>
  ConcreteCandidateSet filter(const TGuessType &guess,
                              const TFeedbackType &feedback,
                              Predicate predicate) const {
     (void)guess; 
     Container filtered;
     filtered.reserve(candidates.size());
     for (const auto &c : candidates) {
       if (predicate(c, feedback)) {
         filtered.push_back(c);
       }
     }
     return ConcreteCandidateSet(std::move(filtered));
  }

  template <typename TGuessType, typename Visitor, typename FeedbackGenerator>
  void visitFeedbackGroups(const TGuessType &guess, Visitor visitor,
                           FeedbackGenerator generator) const {
    using TFeedbackType =
        decltype(generator(std::declval<TCandidateType>(), guess));

    struct Group {
        Container members;
        double score = 0.0;
    };
    std::unordered_map<TFeedbackType, Group> groups;
    
    for (const auto& candidate : candidates) {
        TFeedbackType fb = generator(candidate, guess);
        auto& group = groups[fb];
        group.members.push_back(candidate);
        group.score += getScore(candidate);
    }
    
    for (auto& [fb, group] : groups) {
        visitor(fb, ConcreteCandidateSet(std::move(group.members)), group.score);
    }
  }

private:
  Container candidates;
  double cachedTotalScore = 0.0;

  void recalculateTotalScore() {
    cachedTotalScore = 0.0;
    for (const auto &c : candidates) {
      cachedTotalScore += getScore(c);
    }
  }

  static double getScore(const TCandidateType& c) {
      return c.score; 
  }
};

/**
 * Convenience alias for games where guess type equals solution type (e.g.,
 * Wordle, Mastermind)
 */
template <typename TCandidateType, typename TFeedbackType, typename TConfigType,
          typename TCalculatedGuess, typename TResultType,
          typename TCandidateSet = ConcreteCandidateSet<TCandidateType>>
class AbstractEntSolverSameType
    : public AbstractEntSolver<TCandidateType, TCandidateType, TFeedbackType,
                               TConfigType, TCalculatedGuess, TResultType,
                               TCandidateSet> {
public:
  using Base = AbstractEntSolver<TCandidateType, TCandidateType, TFeedbackType,
                                 TConfigType, TCalculatedGuess, TResultType,
                                 TCandidateSet>;
  using Base::Base;

  // No override needed for calculateGuessProbability as it was removed from base
};
} // namespace Utils