#include "letterBoxed.hpp"
#include <string>
#include <vector>
#include <array>
#include <bitset>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cctype>
#include <unordered_set>

using namespace std;

// --- Out-of-class implementations for header-declared operators ---

bool EquivalenceKey::operator<(const EquivalenceKey &other) const
{
    if (startIndex != other.startIndex)
        return startIndex < other.startIndex;
    if (endIndex != other.endIndex)
        return endIndex < other.endIndex;
    return usedChars.to_ulong() < other.usedChars.to_ulong();
}

std::size_t EquivalenceKeyHash::operator()(const EquivalenceKey &k) const
{
    std::size_t h1 = std::hash<int>()(k.startIndex);
    std::size_t h2 = std::hash<int>()(k.endIndex);
    std::size_t h3 = std::hash<unsigned long>()(k.usedChars.to_ulong());
    return h1 ^ (h2 << 1) ^ (h3 << 2);
}

bool operator==(const EquivalenceKey &a, const EquivalenceKey &b)
{
    return a.startIndex == b.startIndex &&
           a.endIndex == b.endIndex &&
           a.usedChars == b.usedChars;
}

// --- Helper Functions ---

// Converts a string to all lowercase characters.
std::string stringToLower(std::string d)
{
    std::string data;
    data.reserve(d.length());
    for (char c : d)
    {
        data += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return data;
}

// Helper to reconstruct a word string from a WordPath and PuzzleData.
std::string reconstructWordString(const WordPath *wp, const PuzzleData &puzzleData, const std::vector<int> &allPathIndices)
{
    std::string s;
    for (int i = 0; i < wp->indicesLength; ++i)
        s += puzzleData.allLetters[allPathIndices[wp->indicesOffset + i]];
    return s;
}

// Helper to reconstruct a string from a path of WordPath pointers.
std::string reconstructPrintString(const std::vector<const WordPath *> &wordPathPtrs, const PuzzleData &puzzleData, const std::vector<int> &allPathIndices)
{
    std::string printStr = "";
    for (size_t i = 0; i < wordPathPtrs.size(); ++i)
    {
        if (i > 0)
        {
            printStr += " ";
        }
        printStr += reconstructWordString(wordPathPtrs[i], puzzleData, allPathIndices);
    }
    return printStr;
}

// Recursive helper for generating all valid WordPath objects for a given string word.
void findWordPathsRecursive(
    const Word &wordObj,
    const PuzzleData &puzzleData,
    std::vector<WordPath> &results,
    std::vector<int> &currentPathGlobalIndexes,
    int lastUsedSide,
    int depth,
    std::vector<int> &allPathIndices)
{
    if (depth == wordObj.word.length())
    {
        int offset = static_cast<int>(allPathIndices.size());
        allPathIndices.insert(allPathIndices.end(), currentPathGlobalIndexes.begin(), currentPathGlobalIndexes.end());
        results.push_back({offset, static_cast<int>(currentPathGlobalIndexes.size()), puzzleData.letterToSideMapping[currentPathGlobalIndexes.back()], wordObj.order, wordObj.count});
        return;
    }

    char targetChar = wordObj.word[depth];
    for (int globalIdx = 0; globalIdx < puzzleData.allLetters.size(); ++globalIdx)
    {
        if (puzzleData.allLetters[globalIdx] == targetChar)
        {
            int currentSide = puzzleData.letterToSideMapping[globalIdx];
            if (depth > 0 && currentSide == lastUsedSide)
            {
                continue;
            }
            currentPathGlobalIndexes.push_back(globalIdx);
            findWordPathsRecursive(wordObj, puzzleData, results, currentPathGlobalIndexes, currentSide, depth + 1, allPathIndices);
            currentPathGlobalIndexes.pop_back(); // Backtrack
        }
    }
}

// Helper to get the bit index for a char in allLetters
int getLetterBitIndex(char c, const std::array<char, 12> &allLetters)
{
    for (int i = 0; i < 12; ++i)
        if (allLetters[i] == c)
            return i;
    return -1;
}

// Update WordPath to include order (already in header, just use here)

// Update filterWords to take vector<Word> and propagate order to WordPath
std::vector<WordPath> filterWords(const std::vector<Word> &allDictionaryWords, const PuzzleData &puzzleData, const int minLength, const int minUniqueLetters, std::vector<int> &allPathIndices)
{
    std::vector<WordPath> allValidWordPaths;
    allValidWordPaths.reserve(allDictionaryWords.size() / 100);

    for (const Word &wordObj : allDictionaryWords)
    {
        const std::string &word = wordObj.word;
        std::bitset<12> uniqueChars;
        bool containsInvalidCharacter = false;
        for (char c : word)
        {
            int idx = getLetterBitIndex(c, puzzleData.allLetters);
            if (idx == -1)
            {
                containsInvalidCharacter = true;
                break;
            }
            uniqueChars.set(idx);
        }
        if (containsInvalidCharacter)
            continue;
        if (word.length() < minLength)
            continue;
        if ((int)uniqueChars.count() < minUniqueLetters)
            continue;

        std::vector<int> currentPathGlobalIndexes;
        std::vector<WordPath> paths;
        findWordPathsRecursive(wordObj, puzzleData, paths, currentPathGlobalIndexes, -1, 0, allPathIndices);
        allValidWordPaths.insert(allValidWordPaths.end(), paths.begin(), paths.end());
    }
    return allValidWordPaths;
}

// --- Solution Finding (Multi-Stage Process) ---

// STAGE 3: Expands a solution path of classes into all possible string solutions.
void expandAndStoreSolutions(
    const std::vector<const EquivalenceClass *> &classPath,
    std::vector<const WordPath *> &currentWordChain,
    int depth,
    std::vector<Solution> &finalSolutions,
    const PuzzleData &puzzleData,
    const std::vector<int> &allPathIndices)
{
    // Base case: We have selected one word for each class in the path.
    if (depth == classPath.size())
    {
        int orderMin = currentWordChain.empty() ? 0 : currentWordChain[0]->order;
        int orderMax = orderMin;
        int orderSum = 0;
        int countMin = currentWordChain.empty() ? 0 : currentWordChain[0]->count;
        int countMax = countMin;
        int countSum = 0;
        for (const WordPath *wp : currentWordChain)
        {
            if (wp->order < orderMin)
                orderMin = wp->order;
            if (wp->order > orderMax)
                orderMax = wp->order;
            orderSum += wp->order;

            if (wp->count < countMin)
                countMin = wp->count;
            if (wp->count > countMax)
                countMax = wp->count;
            countSum += wp->count;
        }
        finalSolutions.push_back({reconstructPrintString(currentWordChain, puzzleData, allPathIndices),
                                  (int)currentWordChain.size(),
                                  orderMin,
                                  orderSum,
                                  orderMax,
                                  countMin,
                                  countSum,
                                  countMax});
        return;
    }

    // Recursive step: Iterate through all words in the current class.
    const EquivalenceClass *currentClass = classPath[depth];
    for (const WordPath *wordPtr : currentClass->words)
    {
        currentWordChain.push_back(wordPtr);
        expandAndStoreSolutions(classPath, currentWordChain, depth + 1, finalSolutions, puzzleData, allPathIndices);
        currentWordChain.pop_back(); // Backtrack
    }
}

// STAGE 2: Recursively finds solutions using a DFS on Equivalence Classes.
void findClassSolutionsRecursive(
    const EquivalenceClass *lastClass,
    std::vector<const EquivalenceClass *> &currentClassPath, // pass by reference
    std::bitset<12> lettersCovered,
    int currentDepth,
    const int maxDepth,
    bool pruneRedundantPaths,
    const std::vector<EquivalenceClass> &allEqClasses,
    const std::vector<CharStartIndexer> &classIndexers, // Indexer for classes
    const PuzzleData &puzzleData,
    std::vector<std::vector<const EquivalenceClass *>> &classSolutions)
{
    if (currentDepth >= maxDepth)
    {
        return;
    }

    char connectingChar = puzzleData.allLetters[lastClass->key.endIndex];
    int connectingIndex = lastClass->key.endIndex;

    const auto &indexer = classIndexers[connectingChar - 'a'];
    for (int i = indexer.start; i < indexer.end; ++i)
    {
        const EquivalenceClass &nextClass = allEqClasses[i];
        if (nextClass.key.startIndex == connectingIndex)
        {
            // Compute new letters covered by adding nextClass's usedChars
            std::bitset<12> newLettersCovered = lettersCovered | nextClass.key.usedChars;

            // Always prune truly redundant paths, also prune if the next class provides no new letters and optional pruning is enabled.
            if (
                newLettersCovered == lettersCovered &&
                (nextClass.key.endIndex == lastClass->key.endIndex || pruneRedundantPaths))
            {
                continue;
            }

            currentClassPath.push_back(&nextClass);

            if (newLettersCovered == puzzleData.uniquePuzzleLetters)
            {
                classSolutions.push_back(currentClassPath);
            }
            else
            {
                // Continue searching to find longer solutions that might start with the same path.
                findClassSolutionsRecursive(&nextClass, currentClassPath, newLettersCovered, currentDepth + 1, maxDepth, pruneRedundantPaths, allEqClasses, classIndexers, puzzleData, classSolutions);
            }

            currentClassPath.pop_back(); // Backtrack
        }
    }
}

// --- Prune dominated equivalence classes: remove classes that are strictly dominated by another
void pruneDominatedClasses(std::vector<EquivalenceClass> &allEqClasses)
{
    // Group classes by (startIndex, endIndex)
    std::map<std::pair<int, int>, std::vector<size_t>> groups;
    for (size_t i = 0; i < allEqClasses.size(); ++i)
    {
        auto key = std::make_pair(allEqClasses[i].key.startIndex, allEqClasses[i].key.endIndex);
        groups[key].push_back(i);
    }

    std::vector<bool> keep(allEqClasses.size(), true);

    for (const auto &group : groups)
    {
        const auto &indices = group.second;
        for (size_t i = 0; i < indices.size(); ++i)
        {
            for (size_t j = 0; j < indices.size(); ++j)
            {
                if (i == j)
                    continue;
                const auto &a = allEqClasses[indices[i]].key.usedChars;
                const auto &b = allEqClasses[indices[j]].key.usedChars;
                // If B is a strict superset of A, mark A for removal
                if ((b & a) == a && b != a)
                {
                    keep[indices[i]] = false;
                    break;
                }
            }
        }
    }

    // Remove dominated classes
    std::vector<EquivalenceClass> filtered;
    filtered.reserve(allEqClasses.size());
    for (size_t i = 0; i < allEqClasses.size(); ++i)
    {
        if (keep[i])
            filtered.push_back(std::move(allEqClasses[i]));
    }
    allEqClasses = std::move(filtered);
}

// --- Solver Entry Point ---

void runLetterBoxedSolver(
    const PuzzleData &puzzleData,
    const std::vector<Word> &allDictionaryWords,
    const Config &config,
    std::vector<Solution> &finalSolutions)
{
    // Create a vector to hold all character indices for all valid word paths.
    std::vector<int> allPathIndices;
    std::vector<WordPath> allValidWordPaths = filterWords(allDictionaryWords, puzzleData, config.minWordLength, config.minUniqueLetters, allPathIndices);

    // Create equivalence classes based on the valid word paths.
    std::unordered_map<EquivalenceKey, EquivalenceClass, EquivalenceKeyHash> eqClassMap;
    for (const auto &wp : allValidWordPaths)
    {
        EquivalenceKey key;
        key.startIndex = allPathIndices[wp.indicesOffset];
        key.endIndex = allPathIndices[wp.indicesOffset + wp.indicesLength - 1];
        for (int i = 0; i < wp.indicesLength; ++i)
        {
            int idx = allPathIndices[wp.indicesOffset + i];
            key.usedChars.set(idx);
        }
        eqClassMap[key].words.push_back(&wp);
    }
    // Convert the map to a vector of EquivalenceClass for easier processing.
    std::vector<EquivalenceClass> allEqClasses;
    allEqClasses.reserve(eqClassMap.size());
    for (auto &pair : eqClassMap)
    {
        pair.second.key = pair.first;
        allEqClasses.push_back(pair.second);
    }

    // If pruning dominated classes is enabled, remove dominated classes.
    if (config.pruneDominatedClasses)
    {
        pruneDominatedClasses(allEqClasses);
    }

    // Sort equivalence classes by the starting letter for efficient processing.
    std::sort(allEqClasses.begin(), allEqClasses.end(),
              [&](const EquivalenceClass &a, const EquivalenceClass &b)
              {
                  return puzzleData.allLetters[a.key.startIndex] < puzzleData.allLetters[b.key.startIndex];
              });
    // Create a CharStartIndexer to efficiently access classes by their starting character.
    std::vector<CharStartIndexer> classIndexers(26);
    if (!allEqClasses.empty())
    {
        char currentChar = puzzleData.allLetters[allEqClasses[0].key.startIndex];
        classIndexers[currentChar - 'a'].start = 0;
        for (size_t i = 0; i < allEqClasses.size(); ++i)
        {
            char c = puzzleData.allLetters[allEqClasses[i].key.startIndex];
            if (c != currentChar)
            {
                classIndexers[currentChar - 'a'].end = static_cast<int>(i);
                currentChar = c;
                classIndexers[currentChar - 'a'].start = static_cast<int>(i);
            }
        }
        classIndexers[currentChar - 'a'].end = static_cast<int>(allEqClasses.size());
    }

    // Find all solutions by recursively exploring equivalence classes.
    std::vector<std::vector<const EquivalenceClass *>> classSolutions;
    for (const auto &startClass : allEqClasses)
    {
        if (startClass.key.usedChars.count() == puzzleData.uniquePuzzleLetters.count())
        {
            classSolutions.push_back({&startClass});
        }
        std::bitset<12> covered = startClass.key.usedChars;
        std::vector<const EquivalenceClass *> currentClassPath = {&startClass};
        findClassSolutionsRecursive(&startClass, currentClassPath, covered, 1, config.maxDepth, config.pruneRedundantPaths, allEqClasses, classIndexers, puzzleData, classSolutions);
    }

    // Expand each class solution into all possible word paths and store them in finalSolutions.
    for (const auto &classPath : classSolutions)
    {
        std::vector<const WordPath *> currentWordChain;
        expandAndStoreSolutions(classPath, currentWordChain, 0, finalSolutions, puzzleData, allPathIndices);
    }

    std::sort(finalSolutions.begin(), finalSolutions.end(), [](const Solution &a, const Solution &b)
              {
                    if (a.wordCount != b.wordCount) return a.wordCount < b.wordCount;
                    if (a.orderMax != b.orderMax) return a.orderMax < b.orderMax;
                    if (a.countMin != b.countMin) return a.countMin > b.countMin;
                    if (a.countSum != b.countSum) return a.countSum > b.countSum;
                    return a.text < b.text; }); // Sort by text last

    // Deduplicate solutions by their text representation, keeping the first occurrence in sorted order
    std::vector<Solution> deduped;
    std::unordered_set<std::string> seen;
    for (const auto &sol : finalSolutions)
    {
        if (seen.insert(sol.text).second)
        {
            deduped.push_back(sol);
        }
    }
    finalSolutions = std::move(deduped);
}