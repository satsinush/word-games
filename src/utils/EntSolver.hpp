#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
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
 *   - ConfigType: The configuration type (must have feedbackHistory and
 * maxDepth)
 *   - CalculatedGuessType: The type for a guess with calculated ENT
 *   - ResultType: The final result type returned by solve()
 *   - CandidateSetType: The container type for candidates (must support filter,
 * contains, etc.)
 */
template <typename Traits> class AbstractEntSolver {
public:
  struct SearchMetrics {
    double ent = 0.0;
    double wnt = 0.0;
    double probability = 0.0;
  };

  virtual bool isBetterMetrics(const SearchMetrics &a, const SearchMetrics &b,
                               uint32_t R) const {
    const double tolerance = 1e-9;

    bool aGuarantees = (a.wnt > 0.0 && a.wnt <= static_cast<double>(R));
    bool bGuarantees = (b.wnt > 0.0 && b.wnt <= static_cast<double>(R));

    if (aGuarantees != bGuarantees) {
      return aGuarantees;
    }

    if (R <= 1) {
      if (std::abs(a.probability - b.probability) > tolerance) {
        return a.probability > b.probability;
      }
    }

    // Prioritize ENT (average speed) first, then WNT (worst-case speed)
    if (std::abs(a.ent - b.ent) > tolerance) {
      return a.ent < b.ent;
    }

    if (std::abs(a.wnt - b.wnt) > tolerance) {
      return a.wnt < b.wnt;
    }

    if (std::abs(a.probability - b.probability) > tolerance) {
      return a.probability > b.probability;
    }

    return false;
  }

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
  virtual CalculatedGuessType createGuess(const GuessType &guess, double ent,
                                          double wnt,
                                          double probability) const = 0;
  virtual ResultType
  createResult(const std::vector<CalculatedGuessType> &guesses,
               int totalPossible) const = 0;

  // Calculate the probability of a guess being the correct answer
  // Default implementation returns 0.0
  virtual double
  calculateGuessProbability(const GuessType &guess,
                            const CandidateSetType &candidates) const {
    (void)guess;
    (void)candidates;
    return 0.0;
  }

  /// Returns the maximum number of distinct feedback groups (k_max).
  /// This is a physical constant of the game's feedback function:
  ///   Wordle:     3^wordLength (e.g., 243 for 5-letter words)
  ///   Mastermind: (P+1)(P+2)/2 (e.g., 15 for 4 pegs)
  ///   Hangman:    2^unrevealed (positional bitmask)
  ///   Dungleon:   5^5 = 3125
  virtual double maxFeedbackGroups() const { return 2.0; }

  /// Returns the entropy distribution efficiency (alpha) of the game.
  /// Represents how uniformly guesses partition the candidate space.
  ///   Wordle:     0.50 (correlated natural language)
  ///   Mastermind: 0.95 (independent peg colors)
  ///   Hangman:    0.25 (highly correlated positional letter bitmasks)
  ///   Dungleon:   0.60 (highly constrained character combinations)
  virtual double feedbackEfficiency() const { return 1.0; }

  /// Estimates the Expected Number of Turns (ENT) to solve N candidates.
  /// Each loop iteration represents one guess, so the result already
  /// includes the first guess — callers should NOT add +1.
  /// Formula: k_eff = k_max^alpha, B(N) = k_eff*(1-e^(-N/k_eff)),
  /// iterate N ← N/B(N) counting steps until N ≤ 2.
  double estimateENT(size_t numCandidates) const {
    if (numCandidates <= 1)
      return 0.0;

    double N = static_cast<double>(numCandidates);
    double kEff = std::pow(maxFeedbackGroups(), feedbackEfficiency());

    double turns = 0.0;
    while (N > 2.0) {
      double B = kEff * (1.0 - std::exp(-N / kEff));

      if (B <= 1.001) {
        turns += N - 1.0;
        return turns;
      }

      N /= B;
      turns += 1.0;
    }

    // Interpolate final fractional turn for 1.0 < N ≤ 2.0
    turns += (N - 1.0);
    return turns;
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
      filteredCandidates =
          filteredCandidates.filter([this, &feedback](const CandidateType &c) {
            return this->matchesFeedback(c, feedback);
          });

      if (cancellationFlag && cancellationFlag->load()) {
        cancellationFlag = nullptr;
        return createResult(std::vector<CalculatedGuessType>{}, 0);
      }
    }

    std::vector<CalculatedGuessType> guesses;
    int totalPossible = static_cast<int>(filteredCandidates.size());

    activeDepth = config.maxDepth;
    if (config.autoDepth) {
      activeDepth = calculateOptimalDepth(allGuesses, filteredCandidates);
    }

    uint32_t R = (config.maxGuesses > config.feedbackHistory.size())
                     ? (config.maxGuesses - config.feedbackHistory.size())
                     : 1;

    // If activeDepth is 0, skip partition analysis and use the global-state
    // heuristic. Per-guess discrimination would require O(G×C) partition work
    // — the same cost as depth 1 — so if we're at depth 0, just use O(G).
    if (activeDepth == 0) {
      double estEnt = estimateENT(filteredCandidates.size());
      double estWnt = std::ceil(estEnt);
      for (const auto &guess : allGuesses) {
        double prob = calculateGuessProbability(guess, filteredCandidates);
        CalculatedGuessType guessResult =
            createGuess(guess, estEnt, estWnt, prob);
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
                                                 allGuesses, activeDepth, R);
        CalculatedGuessType guess = createGuess(
            guessInput, metrics.ent, metrics.wnt, metrics.probability);
        guesses.push_back(guess);
      }
    }

    std::sort(
        guesses.begin(), guesses.end(),
        [this, R](const CalculatedGuessType &a, const CalculatedGuessType &b) {
          SearchMetrics ma = {a.ent, a.wnt, a.probability};
          SearchMetrics mb = {b.ent, b.wnt, b.probability};

          if (isBetterMetrics(ma, mb, R))
            return true;
          if (isBetterMetrics(mb, ma, R))
            return false;

          // Default fallback / general case sorting using operator<
          return a < b;
        });

    cancellationFlag = nullptr;
    return createResult(guesses, totalPossible);
  }

protected:
  ConfigType config;
  int activeDepth = 0;

private:
  // Cancellation pointer set during solve(); helpers check this and return
  // early
  std::atomic<bool> *cancellationFlag = nullptr;

  /**
   * Returns the minimum total turns to solve candidates from scratch.
   * At leaf (maxDepth=0), delegates to estimateENT.
   * Otherwise, tries all guesses via calculateMetrics and picks the best.
   */
  SearchMetrics findMinMetrics(const CandidateSetType &candidates,
                               const std::vector<GuessType> &allGuesses,
                               const int maxDepth, const uint32_t R) {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    if (candidates.size() <= 1)
      return {0.0, 0.0, 0.0};

    if (maxDepth <= 0) {
      double est = estimateENT(candidates.size());
      return {est, std::ceil(est), 0.0};
    }

    SearchMetrics bestMetrics = {std::numeric_limits<double>::infinity(),
                                 std::numeric_limits<double>::infinity(),
                                 std::numeric_limits<double>::lowest()};

    for (const auto &nextGuess : allGuesses) {
      if (cancellationFlag && cancellationFlag->load())
        return {std::numeric_limits<double>::infinity(),
                std::numeric_limits<double>::infinity(),
                std::numeric_limits<double>::lowest()};

      SearchMetrics metrics =
          calculateMetrics(nextGuess, candidates, allGuesses, maxDepth, R);

      if (isBetterMetrics(metrics, bestMetrics, R)) {
        bestMetrics = metrics;
      }
    }

    return bestMetrics;
  }

  /**
   * Returns total turns (ENT & WNT) when using a specific guess.
   * Equals 1 (this guess) + expected findMinMetrics of resulting subsets.
   * The +1 on line 333 is the ONLY +1 in the scoring chain.
   */
  SearchMetrics calculateMetrics(const GuessType &guessInput,
                                 const CandidateSetType &candidates,
                                 const std::vector<GuessType> &allGuesses,
                                 const int maxDepth, const uint32_t R) {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif

    if (candidates.size() <= 1) {
      return {0.0, 0.0, calculateGuessProbability(guessInput, candidates)};
    }

    double totalScore = candidates.totalScore();
    double expectedRemainingTurns = 0.0;
    double maxSubWnt = 0.0;

    candidates.visitFeedbackGroups(
        guessInput,
        [this, &expectedRemainingTurns, &maxSubWnt, totalScore, &allGuesses,
         maxDepth, R](const FeedbackType &, const CandidateSetType &subset,
                      double subsetScore) {
          double prob = subsetScore / totalScore;
          if (prob > 0) {
            uint32_t nextR = (R > 1) ? (R - 1) : 1;
            SearchMetrics subMetrics =
                findMinMetrics(subset, allGuesses, maxDepth - 1, nextR);
            expectedRemainingTurns += prob * subMetrics.ent;
            if (subMetrics.wnt > maxSubWnt) {
              maxSubWnt = subMetrics.wnt;
            }
          }
        },
        [this](const CandidateType &c, const GuessType &g) {
          return this->generateFeedback(c, g);
        });

    SearchMetrics result = {1.0 + expectedRemainingTurns, 1.0 + maxSubWnt,
                            calculateGuessProbability(guessInput, candidates)};
    return result;
  }

  /**
   * Returns the game's intrinsic branching factor from maxFeedbackGroups().
   * At large N, B_optimal → k_eff, so k_eff is the asymptotic branching factor.
   */
  double getBranchingFactor() const {
    double kMax = maxFeedbackGroups();
    double alpha = feedbackEfficiency();
    return std::max(1.5, std::pow(kMax, alpha));
  }

  /// Microbenchmark generateFeedback for ~100ms; returns estimated ops/ms.
  /// Returns 0.0 if calibration cannot run.
  double measureOpsPerMs(const std::vector<GuessType> &allGuesses,
                         const CandidateSetType &candidates) const {
    if (allGuesses.empty() || candidates.empty()) {
      return 0.0;
    }

    constexpr auto kCalibrateDuration = std::chrono::milliseconds(100);
    const size_t numGuesses = allGuesses.size();

    const auto start = std::chrono::steady_clock::now();
    const auto deadline = start + kCalibrateDuration;

    size_t opCount = 0;
    size_t guessIdx = 0;
    auto candIt = candidates.begin();
    const auto candBegin = candidates.begin();
    const auto candEnd = candidates.end();

    // Virtual generateFeedback cannot be elided; sink keeps the result live.
    volatile size_t sink = 0;

    while (std::chrono::steady_clock::now() < deadline) {
      if (candIt == candEnd) {
        candIt = candBegin;
        guessIdx = (guessIdx + 1) % numGuesses;
      }

      FeedbackType fb = this->generateFeedback(*candIt, allGuesses[guessIdx]);
      sink = sink + 1;
      (void)fb;

      ++candIt;
      ++opCount;
    }

    (void)sink;

    const auto end = std::chrono::steady_clock::now();
    const double elapsedMs =
        std::chrono::duration<double, std::milli>(end - start).count();
    if (elapsedMs <= 0.0 || opCount == 0) {
      return 0.0;
    }

    return static_cast<double>(opCount) / elapsedMs;
  }

  int calculateOptimalDepth(const std::vector<GuessType> &allGuesses,
                            const CandidateSetType &candidates) const {
    const size_t numCandidates = candidates.size();
    const size_t numGuesses = allGuesses.size();

    if (numCandidates <= 1 || numGuesses == 0 || candidates.empty()) {
      return 0;
    }

    constexpr double kBudgetMs = 1000.0;

    const double opsPerMs = measureOpsPerMs(allGuesses, candidates);
    if (opsPerMs <= 0.0) {
      return 0;
    }

    const double threshold = opsPerMs * kBudgetMs;
    const int maxDepth = 3;

    // Get the branching factor B from worstCaseExpectedTurns
    double base = getBranchingFactor();
    if (base < 1.5)
      base = 1.5; // Ensure B has a sensible minimum

    double activeCandidates = static_cast<double>(numCandidates);
    double totalOps = 0.0;
    int resultDepth = maxDepth;

    // Operations at level d grow by G (numGuesses) for each step of depth.
    // At level 1: G * C
    // At level 2: G^2 * C
    // At level 3: G^3 * C
    double levelOps = static_cast<double>(numGuesses) * activeCandidates;

    for (int d = 1; d <= maxDepth; ++d) {
      totalOps += levelOps;

      if (totalOps > threshold) {
        resultDepth = d - 1;
        totalOps -= levelOps; // report ops for the chosen depth only
        break;
      }

      // Next level executes G times more operations
      levelOps *= static_cast<double>(numGuesses);

      // Shrink average candidate subset size for the next level's base check
      activeCandidates /= base;

      // If average candidates per subproblem falls below 1, further depth is
      // wasted
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
 * Provides O(1) contains() and simpler implementation since order doesn't
 * matter.
 */
template <typename TCandidateType> class SetCandidateSet {
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

    for (const auto &candidate : candidates_) {
      TFeedbackType fb = generator(candidate, guess);
      auto &group = groups[fb];
      group.members.insert(candidate);
      group.score += getScore(candidate);
    }

    for (auto &[fb, group] : groups) {
      // Use pre-calculated score to avoid O(n) recalculation
      visitor(fb, SetCandidateSet(std::move(group.members), group.score),
              group.score);
    }
  }

private:
  Container candidates_;
  double cachedTotalScore_ = 0.0;

  static double getScore(const TCandidateType &c) { return c.score; }
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
  double
  calculateGuessProbability(const CandidateType &guess,
                            const CandidateSetType &candidates) const override {
    if (candidates.empty())
      return 0.0;
    return candidates.contains(guess) ? 1.0 / candidates.size() : 0.0;
  }
};
} // namespace Utils