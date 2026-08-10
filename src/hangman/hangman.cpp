#include "hangman/hangman.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

#include "utils/EntSolver.hpp"

namespace Hangman {

std::vector<WordPattern> parsePatternString(const std::string &patternStr) {
  std::vector<WordPattern> patterns;
  std::istringstream iss(patternStr);
  std::string token;

  while (iss >> token) {
    if (!token.empty()) {
      WordPattern wp;
      // Normalize: convert to uppercase for display consistency, but store
      // lowercase internally
      for (char &c : token) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
          c = static_cast<char>(std::tolower(c));
        }
      }
      wp.pattern = token;
      patterns.push_back(wp);
    }
  }

  return patterns;
}

std::string patternsToString(const std::vector<WordPattern> &patterns) {
  std::string result;
  for (size_t i = 0; i < patterns.size(); ++i) {
    if (i > 0)
      result += " ";
    // Convert to uppercase for display
    std::string upper = patterns[i].pattern;
    for (char &c : upper) {
      c = static_cast<char>(std::toupper(c));
    }
    result += upper;
  }
  return result;
}
std::vector<Feedback> parseStrikes(const std::string &strikes) {
  std::vector<Feedback> feedbackList;
  for (char c : strikes) {
    if (std::isalpha(static_cast<unsigned char>(c))) {
      Feedback fb;
      fb.letter = static_cast<char>(std::tolower(c));
      // positions defaults to all zeros (letter not in word)
      feedbackList.push_back(fb);
    }
  }
  return feedbackList;
}

bool matchesPattern(const Utils::Word &word, const WordPattern &pattern, const std::unordered_set<char> &globalRevealed) {
  // Word must have same length as pattern
  if (word.wordString.length() != pattern.length()) {
    return false;
  }

  // Check that revealed letters match strictly at their positions
  std::unordered_set<char> revealedLetters = globalRevealed;
  for (const auto &[pos, letter] : pattern.getRevealedLetters()) {
    if (pos >= word.wordString.length())
      return false;
    if (word.wordString[pos] != letter)
      return false;
    revealedLetters.insert(letter);
  }

  // If a letter is revealed locally or globally, it cannot hide in an
  // unrevealed (non-letter) position in this word slot.
  for (size_t i = 0; i < word.wordString.length(); ++i) {
    char c = static_cast<char>(std::tolower(word.wordString[i]));
    if (revealedLetters.count(c) > 0) {
      if (std::tolower(pattern.pattern[i]) != c) {
        return false; // Rejects "THE" from ___ because 'E' is hidden
      }
    }
  }
  return true;
}

bool matchesFeedback(const PhraseSolution &phrase, const Feedback &fb) {
  std::bitset<64> expected;
  size_t offset = 0;
  for (const auto &word : phrase.words) {
    for (size_t j = 0; j < word.wordString.size(); ++j) {
      if (word.wordString[j] == fb.letter) {
        expected.set(offset + j);
      }
    }
    offset += word.wordString.size();
  }
  return expected == fb.positions;
}

bool matchesWordFeedback(const Utils::Word &word, const Feedback &fb) {
  bool letterInWord = (word.letterCount[fb.letter - 'a'] > 0);
  return letterInWord == fb.isInWord();
}

Feedback generateWordFeedback(const Utils::Word &target, char letter) {
  Feedback fb;
  fb.letter = static_cast<char>(std::tolower(letter));
  for (size_t j = 0; j < target.wordString.size(); ++j) {
    if (target.wordString[j] == fb.letter) {
      fb.positions.set(j);
    }
  }
  return fb;
}

Feedback generateFeedback(const PhraseSolution &target, char letter) {
  Feedback fb;
  fb.letter = static_cast<char>(std::tolower(letter));
  size_t offset = 0;
  for (const auto &word : target.words) {
    for (size_t j = 0; j < word.wordString.size(); ++j) {
      if (word.wordString[j] == fb.letter) {
        fb.positions.set(offset + j);
      }
    }
    offset += word.wordString.size();
  }
  return fb;
}

std::vector<char> getAllLetters() {
  std::vector<char> letters;
  letters.reserve(26);
  for (char c = 'a'; c <= 'z'; ++c) {
    letters.push_back(c);
  }
  return letters;
}

// Defines a set of possible phrase solutions without constructing them all
// explicitly. Uses a product of independent word sets per slot.
class HangmanCandidateSet {
public:
  using Container = std::vector<std::vector<Utils::Word>>;

  HangmanCandidateSet(const Container &wordsPerSlot)
      : wordsPerSlot_(wordsPerSlot) {
    recalculateStats();
  }

  size_t size() const { return cachedSize; }
  bool empty() const { return cachedSize == 0; }
  double totalScore() const { return cachedTotalScore; }

  // Lazy cartesian-product iterator over PhraseSolution values.
  class iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = PhraseSolution;
    using difference_type = std::ptrdiff_t;
    using pointer = const PhraseSolution *;
    using reference = PhraseSolution;

    iterator() = default;

    PhraseSolution operator*() const {
      PhraseSolution phrase;
      if (!slots_ || atEnd_) {
        return phrase;
      }
      phrase.words.reserve(slots_->size());
      double scoreSum = 0.0;
      double minScore = std::numeric_limits<double>::infinity();
      for (size_t i = 0; i < slots_->size(); ++i) {
        const Utils::Word &w = (*slots_)[i][indices_[i]];
        phrase.words.push_back(w);
        scoreSum += w.score;
        if (w.score < minScore) {
          minScore = w.score;
        }
      }
      phrase.score = scoreSum;
      phrase.minScore =
          std::isfinite(minScore) ? minScore : 0.0;
      return phrase;
    }

    iterator &operator++() {
      if (atEnd_ || !slots_) {
        return *this;
      }
      for (int i = static_cast<int>(indices_.size()) - 1; i >= 0; --i) {
        if (++indices_[static_cast<size_t>(i)] <
            (*slots_)[static_cast<size_t>(i)].size()) {
          return *this;
        }
        indices_[static_cast<size_t>(i)] = 0;
      }
      atEnd_ = true;
      return *this;
    }

    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator==(const iterator &other) const {
      if (atEnd_ || other.atEnd_) {
        return atEnd_ == other.atEnd_;
      }
      return slots_ == other.slots_ && indices_ == other.indices_;
    }

    bool operator!=(const iterator &other) const { return !(*this == other); }

  private:
    friend class HangmanCandidateSet;

    iterator(const Container *slots, bool isBegin)
        : slots_(slots), atEnd_(!isBegin) {
      if (!slots_ || slots_->empty()) {
        atEnd_ = true;
        return;
      }
      for (const auto &slot : *slots_) {
        if (slot.empty()) {
          atEnd_ = true;
          return;
        }
      }
      if (isBegin) {
        indices_.assign(slots_->size(), 0);
        atEnd_ = false;
      }
    }

    const Container *slots_ = nullptr;
    std::vector<size_t> indices_;
    bool atEnd_ = true;
  };

  iterator begin() const { return iterator(&wordsPerSlot_, true); }
  iterator end() const { return iterator(&wordsPerSlot_, false); }

  template <typename Predicate> HangmanCandidateSet filter(Predicate) const {
    return *this;
  }

  template <typename Predicate>
  HangmanCandidateSet filter(const char &guess, const Feedback &feedback,
                             Predicate) const {
    (void)guess;
    (void)feedback;
    // Specialized filtering happens via pattern matching before solver
    // invocation. This method exists for interface compliance.
    return *this;
  }

  // Implementation of visitFeedbackGroups using Cartesian product
  // Groups per-slot by positional bitmask, combines across slots with bit offsets
  template <typename Visitor, typename Generator>
  void visitFeedbackGroups(const char &guess, Visitor visitor,
                           Generator /*ignored*/) const {
    // 1. Compute per-slot feedback groups by position bitmask
    std::vector<std::map<uint64_t, std::vector<Utils::Word>>> slotMaps(
        wordsPerSlot_.size());
    std::vector<size_t> slotLengths(wordsPerSlot_.size());

    for (size_t i = 0; i < wordsPerSlot_.size(); ++i) {
      slotLengths[i] = wordsPerSlot_[i].empty()
                           ? 0
                           : wordsPerSlot_[i][0].wordString.size();
      for (const auto &w : wordsPerSlot_[i]) {
        uint64_t mask = 0;
        for (size_t j = 0; j < w.wordString.size(); ++j) {
          if (w.wordString[j] == guess) {
            mask |= (1ULL << j);
          }
        }
        slotMaps[i][mask].push_back(w);
      }
    }

    // 2. Cartesian product with bit offset accumulation
    std::vector<std::vector<Utils::Word>> currentSlotSelection(
        wordsPerSlot_.size());

    auto recurse = [&](auto &&self, size_t index, uint64_t accumMask,
                       size_t bitOffset) -> void {
      if (index == wordsPerSlot_.size()) {
        Feedback fb;
        fb.letter = guess;
        fb.positions = std::bitset<64>(accumMask);

        HangmanCandidateSet subset(currentSlotSelection);
        if (subset.size() > 0) {
          visitor(fb, subset, subset.totalScore());
        }
        return;
      }

      for (const auto &[mask, words] : slotMaps[index]) {
        currentSlotSelection[index] = words;
        self(self, index + 1, accumMask | (mask << bitOffset),
             bitOffset + slotLengths[index]);
      }
    };

    recurse(recurse, 0, 0ULL, 0);
  }

  double probabilityOfLetter(char letter) const {
    double probNotInPhrase = 1.0;
    for (const auto &slot : wordsPerSlot_) {
      double slotTotal = 0.0;
      double slotNoLetter = 0.0;
      int idx = letter - 'a';
      for (const auto &w : slot) {
        slotTotal += w.score;
        if (w.letterCount[idx] == 0) {
          slotNoLetter += w.score;
        }
      }
      if (slotTotal > 0) {
        probNotInPhrase *= (slotNoLetter / slotTotal);
      }
    }
    return 1.0 - probNotInPhrase;
  }

private:
  Container wordsPerSlot_;
  size_t cachedSize = 0;
  double cachedTotalScore = 0.0;

  void recalculateStats() {
    cachedSize = 1;
    cachedTotalScore = 0.0;

    double productOfSums = 1.0;
    bool emptySlot = false;

    if (wordsPerSlot_.empty()) {
      cachedSize = 0;
      cachedTotalScore = 0.0;
      return;
    }

    for (const auto &slot : wordsPerSlot_) {
      size_t s = slot.size();
      if (s == 0)
        emptySlot = true;
      // check overflow?
      cachedSize *= s;

      double slotSum = 0.0;
      for (const auto &w : slot)
        slotSum += w.score;
      productOfSums *= slotSum;
    }

    cachedTotalScore = emptySlot ? 0.0 : productOfSums;
    if (emptySlot)
      cachedSize = 0;
  }
};

// Hangman solver traits - note GuessType (char) differs from CandidateType
// (PhraseSolution)
struct HangmanSolverTraits {
  using CandidateType = PhraseSolution;
  using GuessType = char;
  using FeedbackType = Feedback;
  using ConfigType = Config;
  using CalculatedGuessType = LetterGuess;
  using ResultType = Result;
  using CandidateSetType = HangmanCandidateSet;
};

// Hangman-specific ENT solver implementation
// Uses base AbstractEntSolver since guess type (char) differs from candidate
// type
class HangmanEntSolver : public Utils::AbstractEntSolver<HangmanSolverTraits> {
public:
  HangmanEntSolver(const Config &cfg, size_t numSlots)
      : Utils::AbstractEntSolver<HangmanSolverTraits>(cfg),
        numWordSlots(numSlots) {}

protected:
  using SearchMetrics = typename Utils::AbstractEntSolver<HangmanSolverTraits>::SearchMetrics;

  bool isBetterMetrics(const SearchMetrics &a, const SearchMetrics &b,
                       uint32_t R) const override {
    const double tolerance = 1e-9;

    if (std::abs(a.probability - b.probability) > tolerance) {
      return a.probability > b.probability;
    }

    bool aGuarantees = (a.wnt > 0.0 && a.wnt <= static_cast<double>(R));
    bool bGuarantees = (b.wnt > 0.0 && b.wnt <= static_cast<double>(R));

    if (aGuarantees != bGuarantees) {
      return aGuarantees;
    }

    if (std::abs(a.ent - b.ent) > tolerance) {
      return a.ent < b.ent;
    }

    if (std::abs(a.wnt - b.wnt) > tolerance) {
      return a.wnt < b.wnt;
    }

    return false;
  }

  bool matchesFeedback(const PhraseSolution &candidate,
                       const Feedback &feedback) const override {
    return Hangman::matchesFeedback(candidate, feedback);
  }

  Feedback generateFeedback(const PhraseSolution &target,
                            const char &guess) const override {
    return Hangman::generateFeedback(target, guess);
  }

  double calculateGuessProbability(
      const char &guess, const HangmanCandidateSet &candidates) const override {
    return candidates.probabilityOfLetter(guess);
  }

  LetterGuess createGuess(const char &letter, double ent, double wnt,
                          double probability) const override {
    LetterGuess guess;
    guess.letter = letter;
    guess.ent = ent;
    guess.wnt = wnt;
    guess.probability = probability;
    return guess;
  }

  Result createResult(const std::vector<LetterGuess> &guesses,
                      int totalPossible) const override {
    Result result;
    result.sortedGuesses = guesses;
    result.totalPossiblePatterns = totalPossible;
    result.searchDepth = this->activeDepth;
    return result;
  }

  double maxFeedbackGroups() const override {
    size_t unrevealedCount = 0;
    for (const auto &pattern : this->config.wordPatterns) {
      for (char c : pattern.pattern) {
        if (isUnknownChar(c)) {
          unrevealedCount++;
        }
      }
    }
    return std::pow(2.0, static_cast<double>(unrevealedCount));
  }

  double feedbackEfficiency() const override {
    return 0.25; // Positional bitmask options are highly correlated/sparse
  }

private:
  size_t numWordSlots;
};

Result runHangmanSolver(const Config &config, std::atomic<bool> *cancel) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  Result result;

  // If no patterns specified, return empty result
  if (config.wordPatterns.empty()) {
    result.totalPossiblePatterns = 0;
    return result;
  }

  std::vector<Utils::Word> allWords = Utils::loadWords();
  size_t numSlots = config.wordPatterns.size();

  // Gather ALL globally revealed letters first ---
  std::unordered_set<char> globalRevealedLetters;
  for (const auto &pattern : config.wordPatterns) {
    for (const auto &[pos, letter] : pattern.getRevealedLetters()) {
      globalRevealedLetters.insert(letter);
    }
  }

  // Build list of possible words for each slot (word position)
  std::vector<std::vector<Utils::Word>> wordsPerSlot(numSlots);

  for (size_t slotIdx = 0; slotIdx < numSlots; ++slotIdx) {
    const auto &pattern = config.wordPatterns[slotIdx];
    for (const auto &word : allWords) {
      bool exclude = config.excludeUncommonWords && (!word.is_scrabble) &&
                     word.wordString.length() >
                         1; // Explicitly include 1 letter words for hangman
      if (exclude)
        continue;

      if (matchesPattern(word, pattern, globalRevealedLetters)) {
        // Check feedback history for this word
        bool matches = true;
        for (const auto &fb : config.feedbackHistory) {
          if (!matchesWordFeedback(word, fb)) {
            matches = false;
            break;
          }
        }
        if (matches) {
          wordsPerSlot[slotIdx].push_back(word);
        }
      }
    }
  }

  // Get available letters (exclude guessed or revealed)
  std::unordered_set<char> guessed;
  for (const auto &fb : config.feedbackHistory) {
    guessed.insert(std::tolower(fb.letter));
  }
  for (char c : globalRevealedLetters) {
    guessed.insert(std::tolower(c));
  }

  std::vector<char> availableLetters;
  for (char c = 'a'; c <= 'z'; ++c) {
    if (guessed.count(c) == 0) {
      availableLetters.push_back(c);
    }
  }

  // Filter out empty slots (slots with 0 matching words) if there is at least one slot with words.
  std::vector<std::vector<Utils::Word>> validWordsPerSlot;
  for (const auto &slot : wordsPerSlot) {
    if (!slot.empty()) {
      validWordsPerSlot.push_back(slot);
    }
  }

  bool hasValidSlots = !validWordsPerSlot.empty();
  const auto &solverWordsPerSlot = hasValidSlots ? validWordsPerSlot : wordsPerSlot;

  // Create initial candidate set using the product set approach
  HangmanCandidateSet initialCandidates(solverWordsPerSlot);

  // Use the specialized Hangman ENT solver
  HangmanEntSolver solver(config, solverWordsPerSlot.size());
  result = solver.solve(availableLetters, initialCandidates, cancel);

  // Collect unique possible words for display (from all slots)
  // This is for UI display purposes only
  std::unordered_set<std::string> uniqueWordsForDisplay;
  if (initialCandidates.size() > 0) {
    for (const auto &slot : solverWordsPerSlot) {
      for (const auto &word : slot) {
        uniqueWordsForDisplay.insert(word.wordString);
      }
    }
  }

  std::vector<Utils::Word> possibleWords;
  for (const auto &word : allWords) {
    if (uniqueWordsForDisplay.count(word.wordString) > 0) {
      possibleWords.push_back(word);
    }
  }

  // Store possible words for display
  result.possibleWords = possibleWords;
  result.totalPossiblePatterns = static_cast<int>(initialCandidates.size());

  // Sort by score (higher is better)
  std::sort(result.possibleWords.begin(), result.possibleWords.end(),
            [](const Utils::Word &a, const Utils::Word &b) {
              return a.score > b.score;
            });

  return result;
}

} // namespace Hangman
