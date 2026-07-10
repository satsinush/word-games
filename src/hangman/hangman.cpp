#include "hangman/hangman.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

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

Feedback parseFeedback(const std::string &input) {
  std::istringstream iss(input);
  char letter;
  int inWord;

  if (!(iss >> letter >> inWord)) {
    throw std::invalid_argument(
        "Invalid feedback format. Expected: 'a 1' or 'e 0'");
  }

  Feedback fb;
  fb.letter = static_cast<char>(std::tolower(letter));
  fb.isInWord = (inWord != 0);
  fb.occurrences = fb.isInWord ? 1 : 0;

  return fb;
}

std::vector<Feedback> parseStrikes(const std::string &strikes) {
  std::vector<Feedback> feedbackList;
  for (char c : strikes) {
    if (std::isalpha(static_cast<unsigned char>(c))) {
      Feedback fb;
      fb.letter = static_cast<char>(std::tolower(c));
      fb.isInWord = false;
      fb.occurrences = 0;
      feedbackList.push_back(fb);
    }
  }
  return feedbackList;
}

bool matchesPattern(const Utils::Word &word, const WordPattern &pattern) {
  // Word must have same length as pattern
  if (word.wordString.length() != pattern.length()) {
    return false;
  }

  // Check that revealed letters match and collect them
  std::unordered_set<char> revealedLetters;
  for (const auto &[pos, letter] : pattern.getRevealedLetters()) {
    if (pos >= word.wordString.length())
      return false;
    if (std::tolower(word.wordString[pos]) != letter)
      return false;
    revealedLetters.insert(letter);
  }

  // A letter that has been revealed in the pattern cannot appear in an
  // unrevealed position
  for (size_t i = 0; i < word.wordString.length(); ++i) {
    char c = static_cast<char>(std::tolower(word.wordString[i]));
    if (revealedLetters.count(c) > 0) {
      if (std::tolower(pattern.pattern[i]) != c) {
        return false;
      }
    }
  }

  return true;
}

bool matchesFeedback(const PhraseSolution &phrase, const Feedback &fb) {
  // Count total occurrences of the letter across all words
  size_t totalOccurrences = 0;
  for (const auto &word : phrase.words) {
    totalOccurrences += word.letterCount[fb.letter - 'a'];
  }

  bool letterInPhrase = (totalOccurrences > 0);
  return letterInPhrase == fb.isInWord;
}

bool matchesWordFeedback(const Utils::Word &word, const Feedback &fb) {
  bool letterInWord = (word.letterCount[fb.letter - 'a'] > 0);
  return letterInWord == fb.isInWord;
}

Feedback generateWordFeedback(const Utils::Word &target, char letter) {
  Feedback fb;
  fb.letter = static_cast<char>(std::tolower(letter));
  fb.occurrences = target.letterCount[fb.letter - 'a'];
  fb.isInWord = (fb.occurrences > 0);
  return fb;
}

Feedback generateFeedback(const PhraseSolution &target, char letter) {
  Feedback fb;
  fb.letter = static_cast<char>(std::tolower(letter));

  // Count total occurrences across all words
  size_t totalOccurrences = 0;
  for (const auto &word : target.words) {
    totalOccurrences += word.letterCount[fb.letter - 'a'];
  }

  fb.occurrences = totalOccurrences;
  fb.isInWord = (totalOccurrences > 0);

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

std::vector<char>
getAvailableLetters(const std::vector<Feedback> &feedbackHistory) {
  std::unordered_set<char> guessedLetters;
  for (const auto &fb : feedbackHistory) {
    guessedLetters.insert(fb.letter);
  }

  std::vector<char> available;
  available.reserve(26 - guessedLetters.size());
  for (char c = 'a'; c <= 'z'; ++c) {
    if (guessedLetters.find(c) == guessedLetters.end()) {
      available.push_back(c);
    }
  }
  return available;
}

// Filter words that match a pattern and feedback history
std::vector<Utils::Word>
filterWordsForPattern(const std::vector<Utils::Word> &words,
                      const WordPattern &pattern,
                      [[maybe_unused]] const std::vector<Feedback> &feedbacks) {

  std::vector<Utils::Word> filtered;

  for (const auto &word : words) {
    // Must match pattern length and revealed letters
    if (!matchesPattern(word, pattern)) {
      continue;
    }

    // For single-word filtering, we need a different approach
    // We'll filter phrases later - for now just match pattern
    filtered.push_back(word);
  }

  return filtered;
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
  template <typename Visitor, typename Generator>
  void visitFeedbackGroups(const char &guess, Visitor visitor,
                           Generator /*ignored*/) const {
    // 1. Compute per-slot feedback groups
    // Map "occurrences" (int) -> words.
    std::vector<std::map<int, std::vector<Utils::Word>>> slotMaps(
        wordsPerSlot_.size());

    for (size_t i = 0; i < wordsPerSlot_.size(); ++i) {
      for (const auto &w : wordsPerSlot_[i]) {
        // Generate local feedback (count of letter)
        int count = w.letterCount[guess - 'a'];
        slotMaps[i][count].push_back(w);
      }
    }

    // 2. Cartesian product
    // We need to yield (GlobalFeedback, NewCandidateSet, Score)
    // GlobalFeedback.occurrences = sum(local_counts)
    // GlobalFeedback.isInWord = sum > 0

    std::vector<std::vector<Utils::Word>> currentSlotSelection(
        wordsPerSlot_.size());

    auto recurse = [&](auto &&self, size_t index, int accumCount) -> void {
      if (index == wordsPerSlot_.size()) {
        // Base case
        Feedback fb;
        fb.letter = guess;
        fb.occurrences = static_cast<size_t>(accumCount);
        fb.isInWord = (accumCount > 0);

        HangmanCandidateSet subset(currentSlotSelection);
        if (subset.size() > 0) {
          visitor(fb, subset, subset.totalScore());
        }
        return;
      }

      // Iterate groups in this slot
      for (const auto &[count, words] : slotMaps[index]) {
        currentSlotSelection[index] = words; // copy vector
        self(self, index + 1, accumCount + count);
      }
    };

    recurse(recurse, 0, 0);
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
    result.totalPossibleWords = totalPossible;
    return result;
  }

  double worstCaseExpectedTurns(size_t numCandidates) const override {
    if (numCandidates <= 1)
      return 0.0;
    return std::log2(static_cast<double>(numCandidates));
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
    result.totalPossibleWords = 0;
    return result;
  }

  std::vector<Utils::Word> allWords = Utils::loadWords();
  size_t numSlots = config.wordPatterns.size();

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

      if (matchesPattern(word, pattern)) {
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

  // Get available letters (not yet guessed, and not already revealed in
  // patterns)
  std::vector<char> availableLetters =
      getAvailableLetters(config.feedbackHistory);

  // Also exclude letters that are already revealed in patterns
  std::unordered_set<char> revealedLetters;
  for (const auto &pattern : config.wordPatterns) {
    for (const auto &[pos, letter] : pattern.getRevealedLetters()) {
      revealedLetters.insert(letter);
    }
  }

  // Remove revealed letters from available
  availableLetters.erase(std::remove_if(availableLetters.begin(),
                                        availableLetters.end(),
                                        [&revealedLetters](char c) {
                                          return revealedLetters.count(c) > 0;
                                        }),
                         availableLetters.end());

  // Create initial candidate set using the product set approach
  HangmanCandidateSet initialCandidates(wordsPerSlot);

  // Use the specialized Hangman ENT solver
  HangmanEntSolver solver(config, numSlots);
  result = solver.solve(availableLetters, initialCandidates, cancel);

  // Collect unique possible words for display (from all slots)
  // This is for UI display purposes only
  std::unordered_set<std::string> uniqueWordsForDisplay;
  for (size_t slotIdx = 0; slotIdx < numSlots; ++slotIdx) {
    for (const auto &word : wordsPerSlot[slotIdx]) {
      uniqueWordsForDisplay.insert(word.wordString);
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
  result.totalPossibleWords = static_cast<int>(initialCandidates.size());

  // Sort by score (higher is better)
  std::sort(result.possibleWords.begin(), result.possibleWords.end(),
            [](const Utils::Word &a, const Utils::Word &b) {
              return a.score > b.score;
            });

  return result;
}

} // namespace Hangman
