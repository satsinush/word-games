#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <conio.h>
#include <set>
#include <cctype>
#include <filesystem>
#include <array>
#include <limits>
#include <chrono>
#include "utils.cpp"

using namespace std;

Utils::Profiler profiler;
Utils::Process process = Utils::Process();

// Structure to hold puzzle data (unchanged).
struct PuzzleData
{
    std::array<char, 12> allLetters;
    std::array<int, 12> letterToSideMapping;
    std::set<char> uniquePuzzleLetters;
};

// Represents a single valid word path (unchanged).
struct WordPath
{
    std::string wordString;
    std::vector<int> charGlobalIndexes;
    int lastCharSide;
};

// Structure for storing a final, valid solution.
struct Solution
{
    std::string text; // The full solution string, e.g., "cat tube"
    int wordCount;    // The number of words in the solution.
};

// Indexer for words starting with a specific character (unchanged).
struct CharStartIndexer
{
    int start = 0;
    int end = 0;
};

// --- Helper Functions ---

// Converts a string to all lowercase characters.
std::string stringToLower(std::string d)
{
    std::string data;
    data.reserve(d.length()); // Reserve memory to avoid reallocations.
    for (char c : d)
    {
        data += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return data;
}

// Recursive helper for generating all valid WordPath objects for a given string word.
// It explores all possible sequences of letter global indices that form the word,
// respecting the "no same side consecutively" rule.
void findWordPathsRecursive(
    const std::string &word,
    const PuzzleData &puzzleData,
    std::vector<WordPath> &results,
    std::vector<int> &currentPathGlobalIndexes,
    int lastUsedSide,
    int depth)
{
    // Base case: If we have successfully mapped all characters of the word to global indices.
    if (depth == word.length())
    {
        // A complete WordPath has been found. Store it.
        // The last used side is simply the side of the last character's global index.
        results.push_back({word, currentPathGlobalIndexes, puzzleData.letterToSideMapping[currentPathGlobalIndexes.back()]});
        return;
    }

    char targetChar = word[depth]; // The character from the word we are currently trying to place.

    // Iterate through all 12 global letter indices in the puzzle.
    for (int globalIdx = 0; globalIdx < puzzleData.allLetters.size(); ++globalIdx)
    {
        // Check if the character at the current global index matches the target character.
        if (puzzleData.allLetters[globalIdx] == targetChar)
        {
            int currentSide = puzzleData.letterToSideMapping[globalIdx];

            // Apply the Letter Boxed rule: a character cannot be followed by another character
            // from the same side *within the same word*.
            if (depth > 0 && currentSide == lastUsedSide)
            {
                continue; // Skip: cannot use a letter from the same side consecutively.
            }

            // If valid, add the current global index to the path and recurse.
            currentPathGlobalIndexes.push_back(globalIdx);
            findWordPathsRecursive(word, puzzleData, results, currentPathGlobalIndexes, currentSide, depth + 1);
            currentPathGlobalIndexes.pop_back(); // Backtrack: remove for the next possibility.
        }
    }
}

// Generates all possible WordPath representations for a given word string based on puzzle rules.
// This function acts as a wrapper for the recursive helper.
std::vector<WordPath> getWordPaths(const std::string &word, const PuzzleData &puzzleData, const int minLength)
{
    if (word.length() < minLength)
    {
        return {}; // Word is too short.
    }

    std::vector<WordPath> paths;
    std::vector<int> currentPathGlobalIndexes;
    // Start the recursive search. -1 for lastUsedSide indicates no side has been used yet for the first character.
    findWordPathsRecursive(word, puzzleData, paths, currentPathGlobalIndexes, -1, 0);
    return paths;
}

// Filters the dictionary words and generates all valid WordPath objects for the given puzzle.
// This is the core function for pre-processing the word list.
std::vector<WordPath> filterWords(const std::vector<std::string> &allDictionaryWords, const PuzzleData &puzzleData, const int minLength = 3)
{
    profiler.profileStart("filterWords");
    std::vector<WordPath> allValidWordPaths;
    // Pre-allocate memory, assuming a typical hit rate for words.
    allValidWordPaths.reserve(allDictionaryWords.size() / 100);

    for (const std::string &word : allDictionaryWords)
    {
        // For each word in the dictionary, find all ways it can be formed using the puzzle letters.
        std::vector<WordPath> paths = getWordPaths(word, puzzleData, minLength);
        // Add all found paths for this word to the main list of valid word paths.
        allValidWordPaths.insert(allValidWordPaths.end(), paths.begin(), paths.end());
    }
    profiler.profileEnd("filterWords");
    return allValidWordPaths;
}

// Sorts a vector of WordPath objects by their word string length.
void sortWordPathsByLength(std::vector<WordPath> &wordPaths, const bool ascending = true)
{
    profiler.profileStart("sortWordPathsByLength");
    auto compAscending = [](const WordPath &a, const WordPath &b)
    {
        return a.wordString.length() < b.wordString.length();
    };

    auto compDescending = [](const WordPath &a, const WordPath &b)
    {
        return a.wordString.length() > b.wordString.length();
    };

    if (ascending)
    {
        std::stable_sort(wordPaths.begin(), wordPaths.end(), compAscending);
    }
    else
    {
        std::stable_sort(wordPaths.begin(), wordPaths.end(), compDescending);
    }
    profiler.profileEnd("sortWordPathsByLength");
}

// --- OPTIMIZED RECURSIVE SOLUTION FINDER ---

// Helper to reconstruct the print string from word path indices.
std::string reconstructPrintString(const std::vector<size_t> &wordPathIndices, const std::vector<WordPath> &allValidWordPaths)
{
    std::string printStr = "";
    for (size_t i = 0; i < wordPathIndices.size(); ++i)
    {
        if (i > 0)
        {
            printStr += " ";
        }
        printStr += allValidWordPaths[wordPathIndices[i]].wordString;
    }
    return printStr;
}

/**
 * @brief Recursively finds solutions using a Depth-First Search (DFS) approach.
 *
 * @param lastWordPathIndex The index in `allValidWordPaths` of the word we are extending.
 * @param currentPathIndices The vector of word indices forming the current chain.
 * @param lettersCovered The set of unique characters covered by the current chain.
 * @param currentDepth The current number of words in the chain.
 * @param maxDepth The maximum number of words allowed in a solution.
 * @param allValidWordPaths A constant reference to the master list of all valid word paths.
 * @param puzzleData A constant reference to the puzzle data.
 * @param letterIndexers A constant reference to the start/end indices for words.
 * @param solutions A reference to the vector where final solutions are stored.
 */
void findSolutionsRecursive(
    size_t lastWordPathIndex,
    std::vector<size_t> currentPathIndices,
    std::set<char> lettersCovered,
    int currentDepth,
    const int maxDepth,
    const std::vector<WordPath> &allValidWordPaths,
    const PuzzleData &puzzleData,
    const CharStartIndexer letterIndexers[26],
    std::vector<Solution> &solutions)
{
    // Base case: If we have reached the maximum search depth, stop this path.
    if (currentDepth >= maxDepth)
    {
        return;
    }

    const WordPath &lastWord = allValidWordPaths[lastWordPathIndex];
    char connectingChar = puzzleData.allLetters[lastWord.charGlobalIndexes.back()];
    int connectingIndex = lastWord.charGlobalIndexes.back();

    // Use the pre-computed indexer to find candidate words efficiently.
    const auto &indexer = letterIndexers[connectingChar - 'a'];
    for (int w = indexer.start; w < indexer.end; ++w)
    {
        const WordPath &nextWord = allValidWordPaths[w];

        // Chaining rule: next word must start with the exact same global letter index.
        if (nextWord.charGlobalIndexes[0] == connectingIndex)
        {
            // Create copies for the new recursive path
            std::vector<size_t> newPathIndices = currentPathIndices;
            newPathIndices.push_back(w);

            std::set<char> newLettersCovered = lettersCovered;
            for (size_t i = 1; i < nextWord.charGlobalIndexes.size(); ++i) // Start from 1, as char 0 is the connector
            {
                newLettersCovered.insert(puzzleData.allLetters[nextWord.charGlobalIndexes[i]]);
            }

            // Check if this new chain is a solution.
            if (newLettersCovered.size() == puzzleData.uniquePuzzleLetters.size())
            {
                solutions.push_back({reconstructPrintString(newPathIndices, allValidWordPaths), (int)newPathIndices.size()});
            }
            else
            {
                // Recurse to find longer chains.
                findSolutionsRecursive(w, newPathIndices, newLettersCovered, currentDepth + 1, maxDepth, allValidWordPaths, puzzleData, letterIndexers, solutions);
            }
        }
    }
}

int main()
{
    std::cout << "Reading word file...\n\n";

    std::string file_path = __FILE__;
    std::filesystem::path current_dir = std::filesystem::path(file_path).parent_path();
    std::ifstream file(current_dir / "words.txt");

    std::vector<std::string> allDictionaryWords;
    std::string line;
    if (file.is_open())
    {
        while (std::getline(file, line))
        {
            allDictionaryWords.push_back(line);
        }
        file.close();
        std::cout << "Loaded " << allDictionaryWords.size() << " words from dictionary.\n\n";
    }
    else
    {
        std::cerr << "Error: Could not open words.txt. Please ensure 'words.txt' is in the same directory as the executable.\n";
        std::cout << "\nPress any key to exit.\n";
        _getch(); // Wait for user acknowledgment
        return 1; // Exit with error
    }

    profiler.start();

    std::string side1 = "uvj";
    std::string side2 = "swi";
    std::string side3 = "tge";
    std::string side4 = "bac";
    int minWordLength = 3;
    int maxDepth = 3;

    PuzzleData puzzleData;
    for (int i = 0; i < 3; ++i)
    {
        puzzleData.allLetters[i] = side1[i];
        puzzleData.letterToSideMapping[i] = 0; // Side 0
        puzzleData.uniquePuzzleLetters.insert(side1[i]);
    }
    for (int i = 0; i < 3; ++i)
    {
        puzzleData.allLetters[i + 3] = side2[i];
        puzzleData.letterToSideMapping[i + 3] = 1; // Side 1
        puzzleData.uniquePuzzleLetters.insert(side2[i]);
    }
    for (int i = 0; i < 3; ++i)
    {
        puzzleData.allLetters[i + 6] = side3[i];
        puzzleData.letterToSideMapping[i + 6] = 2; // Side 2
        puzzleData.uniquePuzzleLetters.insert(side3[i]);
    }
    for (int i = 0; i < 3; ++i)
    {
        puzzleData.allLetters[i + 9] = side4[i];
        puzzleData.letterToSideMapping[i + 9] = 3; // Side 3
        puzzleData.uniquePuzzleLetters.insert(side4[i]);
    }

    std::cout << "Puzzle: [" << side1 << "] [" << side2 << "] [" << side3 << "] [" << side4 << "]\n";
    std::cout << "Minimum word length = " << minWordLength << "\n";
    std::cout << "Maximum chain length = " << maxDepth << "\n\n";

    std::cout << "Filtering dictionary for valid words...\n\n";
    std::vector<WordPath> allValidWordPaths = filterWords(allDictionaryWords, puzzleData, minWordLength);

    // Sorting and indexing logic (same as original)
    std::sort(allValidWordPaths.begin(), allValidWordPaths.end(),
              [](const WordPath &a, const WordPath &b)
              { return a.wordString[0] < b.wordString[0]; });

    CharStartIndexer letterIndexers[26];
    for (int k = 0; k < 26; ++k)
    {
        letterIndexers[k].start = 0;
        letterIndexers[k].end = 0;
    }

    if (!allValidWordPaths.empty())
    {
        char currentChar = allValidWordPaths[0].wordString[0];
        letterIndexers[currentChar - 'a'].start = 0;
        for (size_t i = 0; i < allValidWordPaths.size(); ++i)
        {
            if (allValidWordPaths[i].wordString[0] != currentChar)
            {
                letterIndexers[currentChar - 'a'].end = static_cast<int>(i);
                currentChar = allValidWordPaths[i].wordString[0];
                letterIndexers[currentChar - 'a'].start = static_cast<int>(i);
            }
        }
        letterIndexers[currentChar - 'a'].end = static_cast<int>(allValidWordPaths.size());
    }

    std::cout << allValidWordPaths.size() << " valid word path(s) found.\n\n";

    std::cout << "Searching for all possible word chains...\n\n";

    // --- NEW DFS-BASED SEARCH ---
    std::vector<Solution> solutions;

    // Iterate through every valid word as a potential start to a chain.
    for (size_t i = 0; i < allValidWordPaths.size(); ++i)
    {
        const auto &startWord = allValidWordPaths[i];

        // 1. Check if the single word is a solution itself.
        std::set<char> initialLetters;
        for (int globalIdx : startWord.charGlobalIndexes)
        {
            initialLetters.insert(puzzleData.allLetters[globalIdx]);
        }

        if (initialLetters.size() == puzzleData.uniquePuzzleLetters.size())
        {
            solutions.push_back({startWord.wordString, 1});
        }

        // 2. Start the recursive search for longer chains starting with this word.
        findSolutionsRecursive(i, {i}, initialLetters, 1, maxDepth, allValidWordPaths, puzzleData, letterIndexers, solutions);
    }

    profiler.end();
    profiler.logProfilerData();

    // Sort and print the final solutions
    std::sort(solutions.begin(), solutions.end(), [](const Solution &a, const Solution &b)
              {
        if (a.wordCount != b.wordCount)
            return a.wordCount < b.wordCount;
        return a.text < b.text; });

    // Remove duplicate solutions
    solutions.erase(std::unique(solutions.begin(), solutions.end(), [](const Solution &a, const Solution &b)
                                { return a.text == b.text; }),
                    solutions.end());

    for (const auto &s : solutions)
    {
        std::cout << s.text << "\n";
    }
    if (!solutions.empty())
    {
        std::cout << "\n";
    }

    std::cout << solutions.size() << " total solution(s) found.\n";

    std::cout << "\nPress any key to exit.\n";
    _getch();

    return 0;
}