#pragma once
#include <string>
#include <vector>
#include <array>
#include <bitset>

// The main configuration structure for the puzzle solver.
struct Config
{
    std::array<char, 12> letters;
    int minWordLength = 3;
    int minUniqueLetters = 1;
    int maxDepth = 3;
    bool pruneRedundantPaths = true;
    bool pruneDominatedClasses = true;
};

struct PuzzleData
{
    std::array<char, 12> allLetters;
    std::array<int, 12> letterToSideMapping;
    std::bitset<12> uniquePuzzleLetters;
    std::array<int, 256> charToIndexMap;
};

struct WordPath
{
    int indicesOffset;
    int indicesLength;
    int lastCharSide;
    int order;
    int count;
};

struct Solution
{
    std::string text;
    int wordCount;
    int orderMin;
    int orderSum;
    int orderMax;
    int countMin;
    int countSum;
    int countMax;
};

struct CharStartIndexer
{
    int start = 0;
    int end = 0;
};

struct EquivalenceKey
{
    int startIndex;
    int endIndex;
    std::bitset<12> usedChars;
    bool operator<(const EquivalenceKey &other) const;
};

struct EquivalenceKeyHash
{
    std::size_t operator()(const EquivalenceKey &k) const;
};

bool operator==(const EquivalenceKey &a, const EquivalenceKey &b);

struct EquivalenceClass
{
    EquivalenceKey key;
    std::vector<const WordPath *> words;
};

struct Word
{
    std::string word;
    int order;
    int count;
    bool operator<(const Word &other) const { return word < other.word; }
};

void runLetterBoxedSolver(
    const PuzzleData &puzzleData,
    const std::vector<Word> &allDictionaryWords,
    int totalLetterCount,
    const Config &config,
    std::vector<Solution> &finalSolutions);
