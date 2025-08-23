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

#include "utils.hpp"
#include "letterBoxed.hpp"

namespace LetterBoxed
{
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

        // Combine hashes
        std::size_t seed = h1;
        seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }

    bool operator==(const EquivalenceKey &a, const EquivalenceKey &b)
    {
        return a.startIndex == b.startIndex &&
               a.endIndex == b.endIndex &&
               a.usedChars == b.usedChars;
    }

    // --- Helper Functions ---

    // Helper to reconstruct a word string from a WordPath and PuzzleData.
    std::string reconstructWordString(const WordPath *wp, const Config &config, const std::vector<int> &allPathIndices)
    {
        std::string s;
        for (int i = 0; i < wp->indicesLength; ++i)
            s += config.allLetters[allPathIndices[wp->indicesOffset + i]];
        return s;
    }

    // Helper to reconstruct a string from a path of WordPath pointers.
    std::string reconstructPrintString(const std::vector<const WordPath *> &wordPathPtrs, const Config &config, const std::vector<int> &allPathIndices)
    {
        if (wordPathPtrs.empty())
        {
            return "";
        }

        // 1. Calculate final string size
        size_t totalLen = wordPathPtrs.size() - 1; // For spaces between words
        for (const auto *wp : wordPathPtrs)
        {
            totalLen += wp->indicesLength;
        }

        // 2. Reserve and build
        std::string printStr;
        printStr.reserve(totalLen);
        for (size_t i = 0; i < wordPathPtrs.size(); ++i)
        {
            if (i > 0)
            {
                printStr += ' ';
            }
            // Append characters directly
            for (int j = 0; j < wordPathPtrs[i]->indicesLength; ++j)
            {
                printStr += config.allLetters[allPathIndices[wordPathPtrs[i]->indicesOffset + j]];
            }
        }
        return printStr;
    }

    // Recursive helper for generating all valid WordPath objects for a given string word.
    void findWordPathsRecursive(
        const WordUtils::Word &wordObj,
        const Config &config,
        std::vector<WordPath> &results,
        std::vector<int> &currentPathGlobalIndexes,
        int lastUsedSide,
        int depth,
        std::vector<int> &allPathIndices)
    {
        if (depth == wordObj.wordString.length())
        {
            int offset = static_cast<int>(allPathIndices.size());
            allPathIndices.insert(allPathIndices.end(), currentPathGlobalIndexes.begin(), currentPathGlobalIndexes.end());
            results.push_back({offset, static_cast<int>(currentPathGlobalIndexes.size()), config.letterToSideMapping[currentPathGlobalIndexes.back()], wordObj.order, wordObj.count});
            return;
        }

        char targetChar = wordObj.wordString[depth];
        for (int globalIdx = 0; globalIdx < config.allLetters.size(); ++globalIdx)
        {
            if (config.allLetters[globalIdx] == targetChar)
            {
                int currentSide = config.letterToSideMapping[globalIdx];
                if (depth > 0 && currentSide == lastUsedSide)
                {
                    continue;
                }
                currentPathGlobalIndexes.push_back(globalIdx);
                findWordPathsRecursive(wordObj, config, results, currentPathGlobalIndexes, currentSide, depth + 1, allPathIndices);
                currentPathGlobalIndexes.pop_back(); // Backtrack
            }
        }
    }

    // Update WordPath to include order (already in header, just use here)

    // Update filterWords to take vector<Word> and propagate order to WordPath
    void filterWords(std::vector<WordPath> &allValidWordPaths, const std::vector<WordUtils::Word> &allDictionaryWords, const Config &config, std::vector<int> &allPathIndices)
    {
        for (const WordUtils::Word &wordObj : allDictionaryWords)
        {
            const std::string &word = wordObj.wordString;
            std::bitset<12> uniqueChars;
            bool containsInvalidCharacter = false;
            for (char c : word)
            {
                int idx = config.charToIndexMap[static_cast<unsigned char>(c)];
                if (idx == -1)
                {
                    containsInvalidCharacter = true;
                    break;
                }
                uniqueChars.set(idx);
            }
            if (containsInvalidCharacter)
                continue;
            if (word.length() < config.minWordLength)
                continue;
            if ((int)uniqueChars.count() < config.minUniqueLetters)
                continue;

            std::vector<int> currentPathGlobalIndexes;
            currentPathGlobalIndexes.reserve(word.length());
            std::vector<WordPath> paths;
            findWordPathsRecursive(wordObj, config, paths, currentPathGlobalIndexes, -1, 0, allPathIndices);
            allValidWordPaths.insert(allValidWordPaths.end(), paths.begin(), paths.end());
        }
    }

    // --- Solution Finding (Multi-Stage Process) ---

    // STAGE 3: Expands a solution path of classes into all possible string solutions.
    void expandAndStoreSolutions(
        const std::vector<const EquivalenceClass *> &classPath,
        std::vector<const WordPath *> &currentWordChain,
        int depth,
        std::vector<Solution> &finalSolutions,
        const Config &config,
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
            finalSolutions.push_back({reconstructPrintString(currentWordChain, config, allPathIndices),
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
            expandAndStoreSolutions(classPath, currentWordChain, depth + 1, finalSolutions, config, allPathIndices);
            currentWordChain.pop_back(); // Backtrack
        }
    }

    // STAGE 2: Recursively finds solutions using a DFS on Equivalence Classes.
    void findClassSolutionsRecursive(
        const EquivalenceClass *lastClass,
        std::vector<const EquivalenceClass *> &currentClassPath, // pass by reference
        std::bitset<12> lettersCovered,
        int currentDepth,
        const std::vector<EquivalenceClass> &allEqClasses,
        const std::array<CharStartIndexer, 256> &classIndexers,
        const Config &config,
        std::vector<std::vector<const EquivalenceClass *>> &classSolutions)
    {
        if (currentDepth >= config.maxDepth)
        {
            return;
        }

        int end = classIndexers[static_cast<unsigned char>(config.allLetters[lastClass->key.endIndex])].end;
        for (int i = classIndexers[static_cast<unsigned char>(config.allLetters[lastClass->key.endIndex])].start; i < end; ++i)
        {
            const EquivalenceClass &nextClass = allEqClasses[i];
            if (nextClass.key.startIndex == lastClass->key.endIndex)
            {
                // Compute new letters covered by adding nextClass's usedChars
                std::bitset<12> newLettersCovered = lettersCovered | nextClass.key.usedChars;

                // Always prune truly redundant paths, also prune if the next class provides no new letters and optional pruning is enabled.
                if (
                    newLettersCovered == lettersCovered &&
                    (nextClass.key.endIndex == lastClass->key.endIndex || config.pruneRedundantPaths))
                {
                    continue;
                }

                currentClassPath.push_back(&nextClass);

                if (newLettersCovered == config.uniquePuzzleLetters)
                {
                    classSolutions.push_back(currentClassPath);
                }
                else
                {
                    // Continue searching to find longer solutions that might start with the same path.
                    findClassSolutionsRecursive(&nextClass, currentClassPath, newLettersCovered, currentDepth + 1, allEqClasses, classIndexers, config, classSolutions);
                }

                currentClassPath.pop_back(); // Backtrack
            }
        }
    }

    // --- Prune dominated equivalence classes: remove classes that are strictly dominated by another
    void pruneDominatedClasses(std::vector<EquivalenceClass> &allEqClasses)
    {
        // Use unordered_map with combined key for faster grouping
        std::unordered_map<long long, std::vector<size_t>> groups;
        for (size_t i = 0; i < allEqClasses.size(); ++i)
        {
            long long key = (static_cast<long long>(allEqClasses[i].key.startIndex) << 32) | allEqClasses[i].key.endIndex;
            groups[key].push_back(i);
        }

        std::vector<bool> keep(allEqClasses.size(), true);

        for (const auto &group : groups)
        {
            const auto &indices = group.second;
            // Sort indices by popcount of usedChars descending (supersets first)
            std::vector<size_t> sorted = indices;
            std::sort(sorted.begin(), sorted.end(), [&](size_t a, size_t b)
                      { return allEqClasses[a].key.usedChars.count() > allEqClasses[b].key.usedChars.count(); });

            for (size_t i = 0; i < sorted.size(); ++i)
            {
                if (!keep[sorted[i]])
                    continue;
                const auto &a = allEqClasses[sorted[i]].key.usedChars;
                for (size_t j = i + 1; j < sorted.size(); ++j)
                {
                    if (!keep[sorted[j]])
                        continue;
                    const auto &b = allEqClasses[sorted[j]].key.usedChars;
                    // If A is a strict superset of B, mark B for removal
                    if ((a & b) == b && a != b)
                    {
                        keep[sorted[j]] = false;
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

    std::vector<Solution> runLetterBoxedSolver(
        const Config &config,
        const std::vector<WordUtils::Word> &words,
        int totalLetterCount)
    {
        // Create a vector to hold all character indices for all valid word paths.
        std::vector<int> allPathIndices;
        allPathIndices.reserve(totalLetterCount / 100); // Reserve space for indices
        std::vector<WordPath> allValidWordPaths;
        allValidWordPaths.reserve(words.size() / 100);
        filterWords(allValidWordPaths, words, config, allPathIndices);

        // Create equivalence classes based on the valid word paths.
        std::unordered_map<EquivalenceKey, EquivalenceClass, EquivalenceKeyHash> eqClassMap;
        eqClassMap.reserve(allValidWordPaths.size()); // Reserve space for classes
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
                      return config.allLetters[a.key.startIndex] < config.allLetters[b.key.startIndex];
                  });
        // Create a CharStartIndexer to efficiently access classes by their starting character.
        std::array<CharStartIndexer, 256> classIndexers{};
        if (!allEqClasses.empty())
        {
            char currentChar = config.allLetters[allEqClasses[0].key.startIndex];
            classIndexers[static_cast<unsigned char>(currentChar)].start = 0;
            for (size_t i = 0; i < allEqClasses.size(); ++i)
            {
                char c = config.allLetters[allEqClasses[i].key.startIndex];
                if (c != currentChar)
                {
                    classIndexers[static_cast<unsigned char>(currentChar)].end = static_cast<int>(i);
                    currentChar = c;
                    classIndexers[static_cast<unsigned char>(currentChar)].start = static_cast<int>(i);
                }
            }
            classIndexers[static_cast<unsigned char>(currentChar)].end = static_cast<int>(allEqClasses.size());
        }

        // Find all solutions by recursively exploring equivalence classes.
        std::vector<std::vector<const EquivalenceClass *>> classSolutions;
        // classSolutions.reserve(allEqClasses.size() / 10); // Skip reserving, class solutions can vary widely in size
        for (const auto &startClass : allEqClasses)
        {
            if (startClass.key.usedChars.count() == config.uniquePuzzleLetters.count())
            {
                classSolutions.push_back({&startClass});
            }
            std::bitset<12> covered = startClass.key.usedChars;
            std::vector<const EquivalenceClass *> currentClassPath = {&startClass};
            findClassSolutionsRecursive(&startClass, currentClassPath, covered, 1, allEqClasses, classIndexers, config, classSolutions);
        }

        std::vector<Solution> finalSolutions;
        finalSolutions.reserve(classSolutions.size() * 2); // Reserve space for final solutions
        // Expand each class solution into all possible word paths and store them in finalSolutions.
        for (const auto &classPath : classSolutions)
        {
            std::vector<const WordPath *> currentWordChain;
            expandAndStoreSolutions(classPath, currentWordChain, 0, finalSolutions, config, allPathIndices);
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
        deduped.reserve(finalSolutions.size());
        std::unordered_set<std::string> seen;
        seen.reserve(finalSolutions.size());
        for (const auto &sol : finalSolutions)
        {
            if (seen.insert(sol.text).second)
            {
                deduped.push_back(sol);
            }
        }
        return deduped;
    }
}