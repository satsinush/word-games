#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
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
 * 
 * Traits must define:
 *   - CandidateType: The type representing a possible solution
 *   - GuessType: The type representing a guess input
 *   - FeedbackType: The type representing feedback from a guess
 *   - ConfigType: The configuration type (must have feedbackHistory and maxDepth)
 *   - CalculatedGuessType: The type for a guess with calculated ENT
 *   - ResultType: The final result type returned by solve()
 *   - CandidateSetType: The container type for candidates (must support filter, contains, etc.)
 */
template <typename Traits>
class AbstractEntSolver {
public:
  // Extract types from Traits for cleaner usage
  using CandidateType = typename Traits::CandidateType;
  using GuessType = typename Traits::GuessType;
  using FeedbackType = typename Traits::FeedbackType;
  using ConfigType = typename Traits::ConfigType;
  using CalculatedGuessType = typename Traits::CalculatedGuessType;
  using ResultType = typename Traits::ResultType;
  using CandidateSetType = typename Traits::CandidateSetType;

  virtual ~AbstractEntSolver() = default;

  AbstractEntSolver(const ConfigType &config) : config(config) {}

protected:
  // Pure virtual methods to be implemented by derived classes
  virtual bool matchesFeedback(const CandidateType &candidate,
                               const FeedbackType &feedback) const = 0;
  virtual FeedbackType generateFeedback(const CandidateType &target,
                                         const GuessType &guess) const = 0;

  // Pure virtual methods for creating result objects directly
  virtual CalculatedGuessType createGuess(const GuessType &guess, double ent, double probability) const = 0;
  virtual ResultType createResult(const std::vector<CalculatedGuessType> &guesses,
                                   int totalPossible) const = 0;

  // Calculate the probability of a guess being the correct answer
  // Default implementation returns 0.0
  virtual double calculateGuessProbability(const GuessType &guess, const CandidateSetType &candidates) const {
      (void)guess; (void)candidates;
      return 0.0;
  }

  virtual double worstCaseExpectedTurns(size_t numCandidates) const {
    if (numCandidates <= 1) return 0.0;
    return std::log2(static_cast<double>(numCandidates));
  }

public:
  /**
   * Main solver function that returns the final result type directly
   * @param allGuesses All possible guess inputs to evaluate
   * @param initialCandidates The starting set of candidates
   * @param cancel Optional atomic flag to cancel the operation
   */
  virtual ResultType solve(const std::vector<GuessType> &allGuesses,
                    const CandidateSetType &initialCandidates,
                    std::atomic<bool> *cancel = nullptr) {
    cancellationFlag = cancel;

    CandidateSetType filteredCandidates = initialCandidates;
    
    // Apply existing feedback history
    for (const auto &feedback : config.feedbackHistory) {
         filteredCandidates = filteredCandidates.filter([this, &feedback](const CandidateType& c) {
             return this->matchesFeedback(c, feedback);
         });
         
        if (cancellationFlag && cancellationFlag->load()) {
            cancellationFlag = nullptr;
            return createResult(std::vector<CalculatedGuessType>{}, 0);
        }
    }

    std::vector<CalculatedGuessType> guesses;
    int totalPossible = static_cast<int>(filteredCandidates.size());

    // If maxDepth is 0, skip ENT calculation and just return filtered solutions
    if (config.maxDepth == 0) {
      for (const auto &guess : allGuesses) {
        double prob = calculateGuessProbability(guess, filteredCandidates);
        CalculatedGuessType guessResult = createGuess(
            guess, worstCaseExpectedTurns(filteredCandidates.size()), prob);
        guesses.push_back(guessResult);
      }

      std::sort(guesses.begin(), guesses.end());

      return createResult(guesses, totalPossible);
    }

    // Calculate Expected Number of Turns (ENT) for all guesses
    for (const auto &guessInput : allGuesses) {
      if (cancellationFlag && cancellationFlag->load()) {
        cancellationFlag = nullptr;
        return createResult(std::vector<CalculatedGuessType>{}, 0);
      }
      
      double expectedTurns = calculateExpectedTurns(guessInput, filteredCandidates,
                                                    allGuesses, config.maxDepth);

      double prob = calculateGuessProbability(guessInput, filteredCandidates);
      CalculatedGuessType guess = createGuess(guessInput, expectedTurns, prob);
      guesses.push_back(guess);
    }

    std::sort(guesses.begin(), guesses.end());

    cancellationFlag = nullptr;
    return createResult(guesses, totalPossible);
  }

protected:
  ConfigType config;

private:
  // Cancellation pointer set during solve(); helpers check this and return early
  std::atomic<bool> *cancellationFlag = nullptr;

  /**
   * Single-value minimax helper that finds the minimum expected uncertainty
   */
  double findMinExpectedTurns(
      const CandidateSetType &candidates,
      const std::vector<GuessType> &allGuesses,
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
  calculateExpectedTurns(const GuessType &guessInput,
                         const CandidateSetType &candidates,
                         const std::vector<GuessType> &allGuesses,
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
        [this, &expectedRemainingTurns, totalScore, &allGuesses, maxDepth](const FeedbackType&, const CandidateSetType& subset, double subsetScore) {
             
             double prob = subsetScore / totalScore;
             if (prob > 0) {
                 double optimalSubTurns = findMinExpectedTurns(subset, allGuesses, maxDepth - 1);
                 expectedRemainingTurns += prob * optimalSubTurns;
             }
        },
        [this](const CandidateType& c, const GuessType& g) {
            return this->generateFeedback(c, g);
        }
    );

    return 1.0 + expectedRemainingTurns;
  }
};


/**
 * A standard implementation of TCandidateSet using unordered_set.
 * Provides O(1) contains() and simpler implementation since order doesn't matter.
 */
template <typename TCandidateType> 
class SetCandidateSet {
public:
  using Container = std::unordered_set<TCandidateType>;

  SetCandidateSet() = default;
  
  explicit SetCandidateSet(const std::vector<TCandidateType> &candidates) {
    for (const auto &c : candidates) {
      candidates_.insert(c);
      cachedTotalScore_ += getScore(c);
    }
  }
  
  // Move constructor with pre-calculated score (avoids O(n) recalculation)
  SetCandidateSet(Container &&candidates, double totalScore)
      : candidates_(std::move(candidates)), cachedTotalScore_(totalScore) {}

  size_t size() const { return candidates_.size(); }
  bool empty() const { return candidates_.empty(); }
  double totalScore() const { return cachedTotalScore_; }

  auto begin() const { return candidates_.begin(); }
  auto end() const { return candidates_.end(); }

  bool contains(const TCandidateType &candidate) const {
    return candidates_.count(candidate) > 0;
  }

  template <typename Predicate>
  SetCandidateSet filter(Predicate predicate) const {
     Container filtered;
     double filteredScore = 0.0;
     for (const auto &c : candidates_) {
       if (predicate(c)) {
         filtered.insert(c);
         filteredScore += getScore(c);
       }
     }
     return SetCandidateSet(std::move(filtered), filteredScore);
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
    
    for (const auto& candidate : candidates_) {
        TFeedbackType fb = generator(candidate, guess);
        auto& group = groups[fb];
        group.members.insert(candidate);
        group.score += getScore(candidate);
    }
    
    for (auto& [fb, group] : groups) {
        // Use pre-calculated score to avoid O(n) recalculation
        visitor(fb, SetCandidateSet(std::move(group.members), group.score), group.score);
    }
  }

private:
  Container candidates_;
  double cachedTotalScore_ = 0.0;

  static double getScore(const TCandidateType& c) {
      return c.score; 
  }
};

// Backwards compatibility alias
template <typename TCandidateType>
using VectorCandidateSet = SetCandidateSet<TCandidateType>;

/**
 * Convenience class for games where guess type equals candidate type.
 * Provides O(1) probability calculation using the candidate set's hash index.
 * 
 * SameTypeTraits must define same types as AbstractEntSolver Traits,
 * but GuessType must equal CandidateType.
 */
template <typename Traits>
class AbstractEntSolverSameType : public AbstractEntSolver<Traits> {
public:
  using Base = AbstractEntSolver<Traits>;
  using Base::Base;
  
  using CandidateType = typename Traits::CandidateType;
  using CandidateSetType = typename Traits::CandidateSetType;

  // O(1) probability calculation using the candidate set's built-in hash index
  double calculateGuessProbability(const CandidateType &guess, const CandidateSetType &candidates) const override {
      if (candidates.empty()) return 0.0;
      return candidates.contains(guess) ? 1.0 / candidates.size() : 0.0;
  }
};
} // namespace Utils