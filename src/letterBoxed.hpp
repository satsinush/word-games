#pragma once
#include <string>
#include <vector>
#include <array>
#include <bitset>

#include "utils.hpp"

namespace LetterBoxed
{
    // The main configuration structure for the puzzle solver.
    struct Config
    {
        std::array<char, 12> allLetters;
        std::array<int, 12> letterToSideMapping;
        std::bitset<12> uniquePuzzleLetters;
        std::array<int, 256> charToIndexMap;

        int minWordLength = 3;
        int minUniqueLetters = 1;
        int maxDepth = 3;
        bool pruneRedundantPaths = true;
        bool pruneDominatedClasses = true;
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

    std::vector<Solution> runLetterBoxedSolver(
        const Config &config,
        const std::vector<WordUtils::Word> &words,
        int totalLetterCount);
}