#pragma once
#include <array>
#include <bitset>
#include <string>
#include <vector>

#include "utils/inputUtils.hpp"
#include "utils/profilerUtils.hpp"
#include "utils/wordUtils.hpp"

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
  int indicesOffset;
  int indicesLength;
  int lastCharSide;
  double score;
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

struct CharStartIndexer {
  int start = 0;
  int end = 0;
};

struct EquivalenceKey {
  int startIndex;
  int endIndex;
  std::bitset<12> usedChars;
  bool operator<(const EquivalenceKey &other) const;
};

struct EquivalenceKeyHash {
  std::size_t operator()(const EquivalenceKey &k) const;
};

bool operator==(const EquivalenceKey &a, const EquivalenceKey &b);

struct EquivalenceClass {
  EquivalenceKey key;
  std::vector<const WordPath *> words;
};

std::vector<Solution>
runLetterBoxedSolver(const Config &config,
                     const std::vector<Utils::Word> &words);
} // namespace LetterBoxed