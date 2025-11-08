#pragma once
#include <array>
#include <bitset>
#include <string>
#include <vector>

#include "utils/inputUtils.hpp"
#include "utils/utils.hpp"
#include <atomic>

namespace LetterBoxed {
// The main configuration structure for the puzzle solver.
struct Config {
  std::array<char, 12> allLetters;
  std::array<int, 12> letterToSideMapping;
  std::bitset<12> uniquePuzzleLetters;
  std::array<int, 256> charToIndexMap;

  uint8_t minWordLength = 3;
  uint8_t minUniqueLetters = 1;
  uint8_t maxDepth = 3;
  bool pruneRedundantPaths = true;
  bool pruneDominatedClasses = true;
};

struct WordPath {
  std::vector<uint8_t> indices;
  uint8_t lastCharSide;
  Utils::Word
      word; // Store the actual word object for better sorting and information
};

struct Solution {
  std::string text;
  int wordCount;
  double scoreMin;
  double scoreMax;
  double scoreSum;

  bool operator<(const Solution &other) const {
    if (wordCount != other.wordCount)
      return wordCount < other.wordCount;
    if (scoreMin != other.scoreMin)
      return scoreMin > other.scoreMin;
    return text < other.text;
  }
};

struct Result {
  std::vector<Solution> solutions;
  int totalValidWords = 0;
};

struct CharStartIndexer {
  int start = 0;
  int end = 0;
};

struct EquivalenceKey {
  uint8_t startIndex;
  uint8_t endIndex;
  std::bitset<12> usedChars;
  bool operator<(const EquivalenceKey &other) const;
};

struct EquivalenceClass {
  EquivalenceKey key;
  std::vector<const WordPath *> words;
};

Result runLetterBoxedSolver(const Config &config,
                            std::atomic<bool> *cancel = nullptr);
} // namespace LetterBoxed

namespace std {
// Template specialization for std::hash to allow EquivalenceKeyHash in std
// namespace
template <> struct hash<LetterBoxed::EquivalenceKey> {
  std::size_t operator()(const LetterBoxed::EquivalenceKey &k) const {
    std::size_t h1 = std::hash<uint8_t>{}(k.startIndex);
    std::size_t h2 = std::hash<uint8_t>{}(k.endIndex);

    // Hash the bitset
    std::size_t h3 = 0;
    for (size_t i = 0; i < k.usedChars.size(); ++i) {
      h3 ^= (k.usedChars[i] + 0x9e3779b9 + (h3 << 6) + (h3 >> 2));
    }

    // Combine all hashes
    return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2)) ^ h3;
  }
};
} // namespace std