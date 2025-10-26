#include <algorithm>
#include <array>
#include <bitset>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "letterBoxed/letterBoxed.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

namespace LetterBoxed {
// --- Out-of-class implementations for header-declared operators ---

bool EquivalenceKey::operator<(const EquivalenceKey &other) const {
  if (startIndex != other.startIndex)
    return startIndex < other.startIndex;
  if (endIndex != other.endIndex)
    return endIndex < other.endIndex;
  return usedChars.to_ulong() < other.usedChars.to_ulong();
}

bool operator==(const EquivalenceKey &a, const EquivalenceKey &b) {
  return a.startIndex == b.startIndex && a.endIndex == b.endIndex &&
         a.usedChars == b.usedChars;
}

// --- Helper Functions ---

// Helper to reconstruct a string from a path of WordPath pointers.
std::string
reconstructPrintString(const std::vector<const WordPath *> &wordPathPtrs,
                       const Config &config,
                       const std::vector<int> &allPathIndices) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  if (wordPathPtrs.empty()) {
    return "";
  }

  // 1. Calculate final string size
  size_t totalLen = wordPathPtrs.size() - 1; // For spaces between words
  for (const auto *wp : wordPathPtrs) {
    totalLen += wp->indicesLength;
  }

  // 2. Reserve and build
  std::string printStr;
  printStr.reserve(totalLen);
  for (size_t i = 0; i < wordPathPtrs.size(); ++i) {
    if (i > 0) {
      printStr += ' ';
    }
    // Append characters directly
    for (int j = 0; j < wordPathPtrs[i]->indicesLength; ++j) {
      printStr +=
          config.allLetters[allPathIndices[wordPathPtrs[i]->indicesOffset + j]];
    }
  }
  return printStr;
}

// Recursive helper for generating all valid WordPath objects for a given string
// word.
void findWordPathsRecursive(const Utils::Word &wordObj, const Config &config,
                            std::vector<WordPath> &results,
                            std::vector<int> &currentPathGlobalIndexes,
                            const int lastUsedSide, const uint8_t depth,
                            std::vector<int> &allPathIndices,
                            std::atomic<bool> *cancel) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  if (depth == wordObj.wordString.length()) {
    int offset = static_cast<int>(allPathIndices.size());
    allPathIndices.insert(allPathIndices.end(),
                          currentPathGlobalIndexes.begin(),
                          currentPathGlobalIndexes.end());
    results.push_back(
        {offset, static_cast<int>(currentPathGlobalIndexes.size()),
         config.letterToSideMapping[currentPathGlobalIndexes.back()],
         wordObj.score});
    return;
  }

  if (cancel && cancel->load())
    return;

  char targetChar = wordObj.wordString[depth];
  for (unsigned int globalIdx = 0; globalIdx < config.allLetters.size();
       ++globalIdx) {
    if (config.allLetters[globalIdx] == targetChar) {
      int currentSide = config.letterToSideMapping[globalIdx];
      if (depth > 0 && currentSide == lastUsedSide) {
        continue;
      }
      currentPathGlobalIndexes.push_back(globalIdx);
      findWordPathsRecursive(wordObj, config, results, currentPathGlobalIndexes,
                             currentSide, depth + 1, allPathIndices, cancel);
      currentPathGlobalIndexes.pop_back(); // Backtrack
    }
  }
}

// Update WordPath to include order (already in header, just use here)

// Update filterWords to take vector<Word> and propagate order to WordPath
void filterWords(std::vector<WordPath> &allValidWordPaths,
                 const std::vector<Utils::Word> &allDictionaryWords,
                 const Config &config, std::vector<int> &allPathIndices,
                 std::atomic<bool> *cancel) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  for (const Utils::Word &wordObj : allDictionaryWords) {
    if (cancel && cancel->load())
      return;
    const std::string &word = wordObj.wordString;
    std::bitset<12> uniqueChars;
    bool containsInvalidCharacter = false;
    for (char c : word) {
      int idx = config.charToIndexMap[static_cast<unsigned char>(c)];
      if (idx == -1) {
        containsInvalidCharacter = true;
        break;
      }
      uniqueChars.set(idx);
    }
    if (containsInvalidCharacter)
      continue;
    if (word.length() < config.minWordLength)
      continue;
    if (uniqueChars.count() < config.minUniqueLetters)
      continue;

    std::vector<int> currentPathGlobalIndexes;
    currentPathGlobalIndexes.reserve(word.length());
    std::vector<WordPath> paths;
    findWordPathsRecursive(wordObj, config, paths, currentPathGlobalIndexes, -1,
                           0, allPathIndices, cancel);
    allValidWordPaths.insert(allValidWordPaths.end(), paths.begin(),
                             paths.end());
  }
}

// --- Solution Finding (Multi-Stage Process) ---

// STAGE 3: Expands a solution path of classes into all possible string
// solutions.
void expandAndStoreSolutions(
    const std::vector<const EquivalenceClass *> &classPath,
    std::vector<const WordPath *> &currentWordChain, const uint8_t depth,
    std::vector<Solution> &finalSolutions, const Config &config,
    const std::vector<int> &allPathIndices, std::atomic<bool> *cancel) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  // Base case: We have selected one word for each class in the path.
  if (depth == classPath.size()) {
    double scoreMin =
        currentWordChain.empty() ? 0.0 : currentWordChain[0]->score;
    double scoreMax = scoreMin;
    double scoreSum = 0.0;
    for (const WordPath *wp : currentWordChain) {
      if (wp->score < scoreMin)
        scoreMin = wp->score;
      if (wp->score > scoreMax)
        scoreMax = wp->score;
      scoreSum += wp->score;
    }
    finalSolutions.push_back(
        {reconstructPrintString(currentWordChain, config, allPathIndices),
         (int)currentWordChain.size(), scoreMin, scoreMax, scoreSum});
    return;
  }

  if (cancel && cancel->load())
    return;

  // Recursive step: Iterate through all words in the current class.
  const EquivalenceClass *currentClass = classPath[depth];
  for (const WordPath *wordPtr : currentClass->words) {
    if (cancel && cancel->load())
      return;
    currentWordChain.push_back(wordPtr);
    expandAndStoreSolutions(classPath, currentWordChain, depth + 1,
                            finalSolutions, config, allPathIndices, cancel);
    currentWordChain.pop_back(); // Backtrack
  }
}

// STAGE 2: Recursively finds solutions using a DFS on Equivalence Classes.
void findClassSolutionsRecursive(
    const EquivalenceClass *lastClass,
    std::vector<const EquivalenceClass *>
        &currentClassPath, // pass by reference
    const std::bitset<12> lettersCovered, const uint8_t currentDepth,
    const std::vector<EquivalenceClass> &allEqClasses,
    const std::array<CharStartIndexer, 256> &classIndexers,
    const Config &config,
    std::vector<std::vector<const EquivalenceClass *>> &classSolutions,
    std::atomic<bool> *cancel) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  if (currentDepth >= config.maxDepth) {
    return;
  }

  if (cancel && cancel->load())
    return;

  int end = classIndexers[static_cast<unsigned char>(
                              config.allLetters[lastClass->key.endIndex])]
                .end;
  for (int i = classIndexers[static_cast<unsigned char>(
                                 config.allLetters[lastClass->key.endIndex])]
                   .start;
       i < end; ++i) {
    if (cancel && cancel->load())
      return;
    const EquivalenceClass &nextClass = allEqClasses[i];
    if (nextClass.key.startIndex == lastClass->key.endIndex) {
      // Compute new letters covered by adding nextClass's usedChars
      std::bitset<12> newLettersCovered =
          lettersCovered | nextClass.key.usedChars;

      // Always prune truly redundant paths, also prune if the next class
      // provides no new letters and optional pruning is enabled.
      if (newLettersCovered == lettersCovered &&
          (nextClass.key.endIndex == lastClass->key.endIndex ||
           config.pruneRedundantPaths)) {
        continue;
      }

      currentClassPath.push_back(&nextClass);

      if (newLettersCovered == config.uniquePuzzleLetters) {
        classSolutions.push_back(currentClassPath);
      } else {
        // Continue searching to find longer solutions that might start with the
        // same path.
        findClassSolutionsRecursive(
            &nextClass, currentClassPath, newLettersCovered, currentDepth + 1,
            allEqClasses, classIndexers, config, classSolutions, cancel);
      }

      currentClassPath.pop_back(); // Backtrack
    }
  }
}

// --- Prune dominated equivalence classes: remove classes that are strictly
// dominated by another
void pruneDominatedClasses(std::vector<EquivalenceClass> &allEqClasses) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  // Use unordered_map with combined key for faster grouping
  std::unordered_map<long long, std::vector<size_t>> groups;
  for (size_t i = 0; i < allEqClasses.size(); ++i) {
    long long key =
        (static_cast<long long>(allEqClasses[i].key.startIndex) << 32) |
        allEqClasses[i].key.endIndex;
    groups[key].push_back(i);
  }

  std::vector<bool> keep(allEqClasses.size(), true);

  for (const auto &group : groups) {
    const auto &indices = group.second;
    // Sort indices by popcount of usedChars descending (supersets first)
    std::vector<size_t> sorted = indices;
    std::sort(sorted.begin(), sorted.end(), [&](size_t a, size_t b) {
      return allEqClasses[a].key.usedChars.count() >
             allEqClasses[b].key.usedChars.count();
    });

    for (size_t i = 0; i < sorted.size(); ++i) {
      if (!keep[sorted[i]])
        continue;
      const auto &a = allEqClasses[sorted[i]].key.usedChars;
      for (size_t j = i + 1; j < sorted.size(); ++j) {
        if (!keep[sorted[j]])
          continue;
        const auto &b = allEqClasses[sorted[j]].key.usedChars;
        // If A is a strict superset of B, mark B for removal
        if ((a & b) == b && a != b) {
          keep[sorted[j]] = false;
        }
      }
    }
  }

  // Remove dominated classes
  std::vector<EquivalenceClass> filtered;
  filtered.reserve(allEqClasses.size());
  for (size_t i = 0; i < allEqClasses.size(); ++i) {
    if (keep[i])
      filtered.push_back(std::move(allEqClasses[i]));
  }
  allEqClasses = std::move(filtered);
}

// --- Solver Entry Point ---

std::vector<Solution>
runLetterBoxedSolver(const Config &config,
                     const std::vector<Utils::Word> &words,
                     std::atomic<bool> *cancel) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  // Create a vector to hold all character indices for all valid word paths.
  std::vector<int> allPathIndices;
  allPathIndices.reserve(words.size() / 100); // Reserve space for indices
  std::vector<WordPath> allValidWordPaths;
  allValidWordPaths.reserve(words.size() / 100);
  filterWords(allValidWordPaths, words, config, allPathIndices, cancel);

  // Create equivalence classes based on the valid word paths.
  std::unordered_map<EquivalenceKey, EquivalenceClass> eqClassMap;
  eqClassMap.reserve(allValidWordPaths.size()); // Reserve space for classes
  for (const auto &wp : allValidWordPaths) {
    EquivalenceKey key;
    key.startIndex = allPathIndices[wp.indicesOffset];
    key.endIndex = allPathIndices[wp.indicesOffset + wp.indicesLength - 1];
    for (int i = 0; i < wp.indicesLength; ++i) {
      int idx = allPathIndices[wp.indicesOffset + i];
      key.usedChars.set(idx);
    }
    eqClassMap[key].words.push_back(&wp);
  }
  // Convert the map to a vector of EquivalenceClass for easier processing.
  std::vector<EquivalenceClass> allEqClasses;
  allEqClasses.reserve(eqClassMap.size());
  for (auto &pair : eqClassMap) {
    pair.second.key = pair.first;
    allEqClasses.push_back(pair.second);
  }

  // If pruning dominated classes is enabled, remove dominated classes.
  if (config.pruneDominatedClasses) {
    pruneDominatedClasses(allEqClasses);
  }

  // Sort equivalence classes by the starting letter for efficient processing.
  std::sort(allEqClasses.begin(), allEqClasses.end(),
            [&](const EquivalenceClass &a, const EquivalenceClass &b) {
              return config.allLetters[a.key.startIndex] <
                     config.allLetters[b.key.startIndex];
            });
  // Create a CharStartIndexer to efficiently access classes by their starting
  // character.
  std::array<CharStartIndexer, 256> classIndexers{};
  if (!allEqClasses.empty()) {
    char currentChar = config.allLetters[allEqClasses[0].key.startIndex];
    classIndexers[static_cast<unsigned char>(currentChar)].start = 0;
    for (size_t i = 0; i < allEqClasses.size(); ++i) {
      char c = config.allLetters[allEqClasses[i].key.startIndex];
      if (c != currentChar) {
        classIndexers[static_cast<unsigned char>(currentChar)].end =
            static_cast<int>(i);
        currentChar = c;
        classIndexers[static_cast<unsigned char>(currentChar)].start =
            static_cast<int>(i);
      }
    }
    classIndexers[static_cast<unsigned char>(currentChar)].end =
        static_cast<int>(allEqClasses.size());
  }

  // Find all solutions by recursively exploring equivalence classes.
  std::vector<std::vector<const EquivalenceClass *>> classSolutions;
  // classSolutions.reserve(allEqClasses.size() / 10); // Skip reserving, class
  // solutions can vary widely in size
  for (const auto &startClass : allEqClasses) {
    if (startClass.key.usedChars.count() ==
        config.uniquePuzzleLetters.count()) {
      classSolutions.push_back({&startClass});
    }
    std::bitset<12> covered = startClass.key.usedChars;
    std::vector<const EquivalenceClass *> currentClassPath = {&startClass};
    findClassSolutionsRecursive(&startClass, currentClassPath, covered, 1,
                                allEqClasses, classIndexers, config,
                                classSolutions, cancel);
  }

  std::vector<Solution> finalSolutions;
  // Reserve space for final solutions
  finalSolutions.reserve(classSolutions.size() * 2);
  // Expand each class solution into all possible word paths and store them in
  // finalSolutions.
  for (const auto &classPath : classSolutions) {
    std::vector<const WordPath *> currentWordChain;
    expandAndStoreSolutions(classPath, currentWordChain, 0, finalSolutions,
                            config, allPathIndices, cancel);
  }

  std::sort(finalSolutions.begin(), finalSolutions.end());

  // Deduplicate solutions by their text representation
  // Multiple internal paths can produce identical solution strings with
  // identical scores
  std::vector<Solution> deduped;
  deduped.reserve(finalSolutions.size());
  std::unordered_set<std::string> seen;
  seen.reserve(finalSolutions.size());
  for (const auto &sol : finalSolutions) {
    if (seen.insert(sol.text).second) {
      deduped.push_back(sol);
    }
  }
  return deduped;
}
} // namespace LetterBoxed