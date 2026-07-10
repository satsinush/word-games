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
  virtual CalculatedGuessType createGuess(const GuessType &guess, double ent, double wnt, double probability) const = 0;
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

    int activeDepth = config.maxDepth;
    if (config.autoDepth) {
      activeDepth = calculateOptimalDepth(allGuesses.size(), filteredCandidates.size());
    }

    uint32_t R = (config.maxGuesses > config.feedbackHistory.size())
                 ? (config.maxGuesses - config.feedbackHistory.size())
                 : 1;

    // If activeDepth is 0, skip ENT calculation and just return filtered solutions
    if (activeDepth == 0) {
      double estEnt = worstCaseExpectedTurns(filteredCandidates.size());
      double estWnt = std::ceil(estEnt);
      for (const auto &guess : allGuesses) {
        double prob = calculateGuessProbability(guess, filteredCandidates);
        CalculatedGuessType guessResult = createGuess(guess, estEnt, estWnt, prob);
        guesses.push_back(guessResult);
      }
    } else {
      // Calculate Expected Number of Turns (ENT) and WNT for all guesses
      for (const auto &guessInput : allGuesses) {
        if (cancellationFlag && cancellationFlag->load()) {
          cancellationFlag = nullptr;
          return createResult(std::vector<CalculatedGuessType>{}, 0);
        }
        
        SearchMetrics metrics = calculateMetrics(guessInput, filteredCandidates,
                                                      allGuesses, activeDepth);
        double prob = calculateGuessProbability(guessInput, filteredCandidates);
        CalculatedGuessType guess = createGuess(guessInput, metrics.ent, metrics.wnt, prob);
        guesses.push_back(guess);
      }
    }

    // Check if there is any guess that guarantees a win (i.e. WNT <= R)
    bool guaranteeExists = false;
    for (const auto &g : guesses) {
        if (g.wnt > 0.0 && g.wnt <= static_cast<double>(R)) {
            guaranteeExists = true;
            break;
        }
    }

    std::sort(guesses.begin(), guesses.end(), [R, guaranteeExists](const CalculatedGuessType &a, const CalculatedGuessType &b) {
        const double tolerance = 1e-9;

        if (R <= 1) {
            // Last guess: prioritize individual probability first
            if (std::abs(a.probability - b.probability) > tolerance)
                return a.probability > b.probability;
            
            if (std::abs(a.wnt - b.wnt) > tolerance)
                return a.wnt < b.wnt;

            if (std::abs(a.ent - b.ent) > tolerance)
                return a.ent < b.ent;
        } else if (guaranteeExists) {
            // If any guess guarantees a win (WNT <= R), we ONLY want guesses with WNT <= R.
            // If one guarantees a win and the other does not, prefer the one that does.
            bool aGuarantees = (a.wnt > 0.0 && a.wnt <= static_cast<double>(R));
            bool bGuarantees = (b.wnt > 0.0 && b.wnt <= static_cast<double>(R));
            if (aGuarantees != bGuarantees) {
                return aGuarantees;
            }
            if (std::abs(a.ent - b.ent) > tolerance)
                return a.ent < b.ent;
            
            if (std::abs(a.probability - b.probability) > tolerance)
                return a.probability > b.probability;

            if (std::abs(a.wnt - b.wnt) > tolerance)
                return a.wnt < b.wnt;
        } else {
            // No guarantee: prioritize WNT first (survival mode), then ENT
            if (std::abs(a.wnt - b.wnt) > tolerance)
                return a.wnt < b.wnt;

            if (std::abs(a.ent - b.ent) > tolerance)
                return a.ent < b.ent;

            if (std::abs(a.probability - b.probability) > tolerance)
                return a.probability > b.probability;
        }

        // Single fallback tiebreaker using the types' own operator<
        return a < b;
    });

    cancellationFlag = nullptr;
    return createResult(guesses, totalPossible);
  }

protected:
  ConfigType config;

private:
  // Cancellation pointer set during solve(); helpers check this and return early
  std::atomic<bool> *cancellationFlag = nullptr;

  struct SearchMetrics {
      double ent = 0.0;
      double wnt = 0.0;
  };

  /**
   * Single-value minimax helper that finds the minimum expected and worst-case turns
   */
  SearchMetrics findMinMetrics(
      const CandidateSetType &candidates,
      const std::vector<GuessType> &allGuesses,
      const int maxDepth) {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    if (candidates.size() <= 1)
      return {0.0, 0.0};

    if (maxDepth <= 0) {
      double est = worstCaseExpectedTurns(candidates.size());
      return {est, std::ceil(est)};
    }

    double minWnt = std::numeric_limits<double>::max();
    double bestEntForMinWnt = std::numeric_limits<double>::max();

    for (const auto &nextGuess : allGuesses) {
      if (cancellationFlag && cancellationFlag->load())
        return {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};

      SearchMetrics metrics = calculateMetrics(nextGuess, candidates, allGuesses, maxDepth);
      
      // Minimize WNT as primary, and ENT as secondary tiebreaker
      if (metrics.wnt < minWnt) {
          minWnt = metrics.wnt;
          bestEntForMinWnt = metrics.ent;
      } else if (std::abs(metrics.wnt - minWnt) < 1e-9) {
          if (metrics.ent < bestEntForMinWnt) {
              bestEntForMinWnt = metrics.ent;
          }
      }
    }

    return {bestEntForMinWnt, minWnt};
  }

  /**
   * Calculates the Expected and Worst-case Number of Turns (ENT & WNT)
   * after making a specific guess.
   */
  SearchMetrics calculateMetrics(
      const GuessType &guessInput,
      const CandidateSetType &candidates,
      const std::vector<GuessType> &allGuesses,
      const int maxDepth) {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif

    if (candidates.size() <= 1) {
        return {0.0, 0.0}; 
    }

    double totalScore = candidates.totalScore();
    double expectedRemainingTurns = 0.0;
    double maxSubWnt = 0.0;
    
    candidates.visitFeedbackGroups(
        guessInput,
        [this, &expectedRemainingTurns, &maxSubWnt, totalScore, &allGuesses, maxDepth](const FeedbackType&, const CandidateSetType& subset, double subsetScore) {
             double prob = subsetScore / totalScore;
             if (prob > 0) {
                 SearchMetrics subMetrics = findMinMetrics(subset, allGuesses, maxDepth - 1);
                 expectedRemainingTurns += prob * subMetrics.ent;
                 if (subMetrics.wnt > maxSubWnt) {
                     maxSubWnt = subMetrics.wnt;
                 }
             }
        },
        [this](const CandidateType& c, const GuessType& g) {
            return this->generateFeedback(c, g);
        }
    );

    return {1.0 + expectedRemainingTurns, 1.0 + maxSubWnt};
  }

  /**
   * Dynamically extracts the game's intrinsic branching factor (B) by reversing the
   * logarithmic formula implemented in the subclass's worstCaseExpectedTurns.
   *
   * Derivation:
   *   T = worstCaseExpectedTurns(C) = ln(C) / ln(B)
   *   ln(B) = ln(C) / T
   *   B = exp(ln(C) / T)
   * We use C = 128 (a power of 2) as a dummy candidate size to solve for B.
   */
  double getBranchingFactor() const {
      double turns = worstCaseExpectedTurns(128);
      if (turns <= 0.0) return 2.0; // Fallback safe base
      return std::exp(std::log(128.0) / turns);
  }

  int calculateOptimalDepth(size_t numGuesses, size_t numCandidates) const {
      if (numCandidates <= 1) {
          return 0;
      }
      
      const double threshold = 3e7; // 30 million operations
      const int maxDepth = 3;
      
      // Get the branching factor B from worstCaseExpectedTurns
      double base = getBranchingFactor();
      if (base < 1.5) base = 1.5; // Ensure B has a sensible minimum
      
      double activeCandidates = static_cast<double>(numCandidates);
      double totalOps = 0.0;
      int resultDepth = maxDepth;
      
      // Operations at level d grow by the number of guesses G tested at each node,
      // and shrink by the branching factor B (base) of candidate subsets.
      // Net scale factor per level = G / B
      double levelOps = static_cast<double>(numGuesses) * activeCandidates;
      double scaleFactor = static_cast<double>(numGuesses) / base;
      
      for (int d = 1; d <= maxDepth; ++d) {
          totalOps += levelOps;
          
          if (totalOps > threshold) {
              resultDepth = d - 1;
              break;
          }
          
          levelOps *= scaleFactor;
          
          // Shrink average candidate subset size for the next level
          activeCandidates /= base;
          
          // If average candidates per subproblem falls below 1, further depth is wasted
          if (activeCandidates <= 1.0) {
              resultDepth = d;
              break;
          }
      }

      return resultDepth;
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