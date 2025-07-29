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
#include <map>
#include <bitset>
#include <unordered_map>
#include "utils.cpp"

using namespace std;

Utils::Profiler profiler;
Utils::Process process = Utils::Process();

// --- Core Data Structures ---

struct PuzzleData
{
    std::array<char, 12> allLetters;
    std::array<int, 12> letterToSideMapping;
    std::bitset<12> uniquePuzzleLetters; // bitset instead of set<char>
};

struct WordPath
{
    int indicesOffset; // Offset into the master vector
    int indicesLength; // Number of indices for this word
    int lastCharSide;
};

struct Solution
{
    std::string text; // The full solution string, e.g., "cat tube"
    int wordCount;    // The number of words in the solution.
};

// Indexer for words or classes starting with a specific character.
struct CharStartIndexer
{
    int start = 0;
    int end = 0;
};

// --- Equivalence Class Structures ---

/**
 * @brief A key to uniquely identify an equivalence class.
 *
 * Two WordPaths are equivalent if they have the same start/end global indices
 * and use the same set of unique characters.
 */
struct EquivalenceKey
{
    int startIndex;
    int endIndex;
    std::bitset<12> usedChars; // bitset instead of set<char>

    // operator< is required to use this struct as a key in std::map.
    bool operator<(const EquivalenceKey &other) const
    {
        if (startIndex != other.startIndex)
            return startIndex < other.startIndex;
        if (endIndex != other.endIndex)
            return endIndex < other.endIndex;
        return usedChars.to_ulong() < other.usedChars.to_ulong();
    }
};

// --- Helper for hashing and equality for EquivalenceKey for unordered_map ---
struct EquivalenceKeyHash
{
    std::size_t operator()(const EquivalenceKey &k) const
    {
        std::size_t h1 = std::hash<int>()(k.startIndex);
        std::size_t h2 = std::hash<int>()(k.endIndex);
        std::size_t h3 = std::hash<unsigned long>()(k.usedChars.to_ulong());
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

// Provide == operator for EquivalenceKey for unordered_map
inline bool operator==(const EquivalenceKey &a, const EquivalenceKey &b)
{
    return a.startIndex == b.startIndex &&
           a.endIndex == b.endIndex &&
           a.usedChars == b.usedChars;
}

/**
 * @brief Represents a class of equivalent words.
 *
 * All words in this class share the same key properties and can be used
 * interchangeably in the first stage of the solution search.
 */
struct EquivalenceClass
{
    EquivalenceKey key;
    std::vector<const WordPath *> words; // Pointers to the actual words in this class
};

// --- New Struct for User Configuration ---
struct Config
{
    std::array<char, 12> letters;
    int minWordLength = 3;
    int minUniqueLetters = 1;
    int maxDepth = 3;
    bool pruneRedundantPaths = true;
    bool pruneDominatedClasses = true; // <--- Add this parameter
};

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
    const std::string &word,
    const PuzzleData &puzzleData,
    std::vector<WordPath> &results,
    std::vector<int> &currentPathGlobalIndexes,
    int lastUsedSide,
    int depth,
    std::vector<int> &allPathIndices)
{
    if (depth == word.length())
    {
        int offset = static_cast<int>(allPathIndices.size());
        allPathIndices.insert(allPathIndices.end(), currentPathGlobalIndexes.begin(), currentPathGlobalIndexes.end());
        results.push_back({offset, static_cast<int>(currentPathGlobalIndexes.size()), puzzleData.letterToSideMapping[currentPathGlobalIndexes.back()]});
        return;
    }

    char targetChar = word[depth];
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
            findWordPathsRecursive(word, puzzleData, results, currentPathGlobalIndexes, currentSide, depth + 1, allPathIndices);
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

// Generates all possible WordPath representations for a given word string.
std::vector<WordPath> getWordPaths(const std::string &word, const PuzzleData &puzzleData, const int minLength, const int minUniqueLetters, std::vector<int> &allPathIndices)
{
    if (word.length() < minLength)
    {
        return {};
    }
    std::bitset<12> uniqueChars;
    for (char c : word)
    {
        int idx = getLetterBitIndex(c, puzzleData.allLetters);
        if (idx != -1)
            uniqueChars.set(idx);
    }
    if ((int)uniqueChars.count() < minUniqueLetters)
    {
        return {};
    }
    std::vector<WordPath> paths;
    std::vector<int> currentPathGlobalIndexes;
    findWordPathsRecursive(word, puzzleData, paths, currentPathGlobalIndexes, -1, 0, allPathIndices);
    return paths;
}

// Filters the dictionary words and generates all valid WordPath objects for the puzzle.
// Now includes a pre-filter to discard words with letters not in the puzzle.
std::vector<WordPath> filterWords(const std::vector<std::string> &allDictionaryWords, const PuzzleData &puzzleData, const int minLength, const int minUniqueLetters, std::vector<int> &allPathIndices)
{
    std::set<char> puzzleLetterSet;
    for (char c : puzzleData.allLetters)
        puzzleLetterSet.insert(c);

    std::vector<WordPath> allValidWordPaths;
    allValidWordPaths.reserve(allDictionaryWords.size() / 100);

    for (const std::string &word : allDictionaryWords)
    {
        bool valid = true;
        for (char c : word)
        {
            if (puzzleLetterSet.find(c) == puzzleLetterSet.end())
            {
                valid = false;
                break;
            }
        }
        if (!valid)
            continue;

        std::vector<WordPath> paths = getWordPaths(word, puzzleData, minLength, minUniqueLetters, allPathIndices);
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
        finalSolutions.push_back({reconstructPrintString(currentWordChain, puzzleData, allPathIndices), (int)currentWordChain.size()});
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
            if (pruneRedundantPaths)
            {
                // Check if nextClass provides any new letters
                std::bitset<12> newLetters = nextClass.key.usedChars & ~lettersCovered;
                if (newLetters.none())
                {
                    // All letters already covered, skip unless you want to allow cycles/extensions
                    continue;
                }
            }

            std::bitset<12> newLettersCovered = lettersCovered | nextClass.key.usedChars;

            currentClassPath.push_back(&nextClass);

            if (newLettersCovered.count() == puzzleData.uniquePuzzleLetters.count())
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

// --- Console and Input Helper Functions ---

void drawPuzzleBox(const std::array<char, 12> &letters)
{
#ifdef _WIN32
    system("cls");
#else
    cout << "\033[2J\033[1;1H";
#endif

    cout << "Enter the 12 letters for the puzzle. Use Backspace to correct." << endl
         << endl;

    // Draw letters around an empty box, displayed as uppercase
    auto up = [](char c)
    { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); };

    cout << "     " << up(letters[0]) << " " << up(letters[1]) << " " << up(letters[2]) << endl;
    cout << "   +-------+" << endl;
    cout << " " << up(letters[11]) << " |       | " << up(letters[3]) << endl;
    cout << " " << up(letters[10]) << " |       | " << up(letters[4]) << endl;
    cout << " " << up(letters[9]) << " |       | " << up(letters[5]) << endl;
    cout << "   +-------+" << endl;
    cout << "     " << up(letters[8]) << " " << up(letters[7]) << " " << up(letters[6]) << endl;
    cout << endl
         << endl;
}

// Gathers all puzzle settings from the user.
Config getUserConfiguration()
{
    Config config;
    std::array<char, 12> letters;
    letters.fill('_'); // Initialize with placeholders

    int currentIndex = 0;
    bool done = false;
    while (!done)
    {
        for (int i = currentIndex + 1; i < 12; i++)
        {
            letters[i] = '*'; // Reset unused letters
        }
        letters[currentIndex] = '_'; // Reset current letter
        drawPuzzleBox(letters);

        if (currentIndex >= 12 && !done)
        {
            // Ask for confirmation
            cout << "Press enter to confirm: ";
        }

        char input = _getch();

        if (isalpha(input) && currentIndex < 12)
        {
            letters[currentIndex] = tolower(input);
            currentIndex++;
        }
        if (input == 8)
        { // ASCII for Backspace
            if (currentIndex > 0)
            {
                currentIndex--;
                letters[currentIndex] = '_';
            }
        }
        else if ((input == '\r' || input == '\n') && currentIndex >= 12)
        {
            done = true;
        }
    }

    config.letters = letters;
    drawPuzzleBox(config.letters); // Final draw

    // Get other settings
    cout << "Enter minimum word length (e.g., 3): ";
    cin >> config.minWordLength;
    cout << "Enter minimum unique letters per word (e.g., 2): ";
    cin >> config.minUniqueLetters;
    cout << "Enter max solution depth (e.g., 3 words): ";
    cin >> config.maxDepth;
    cout << "Prune redundant paths? (1 = yes, 0 = no): ";
    cin >> config.pruneRedundantPaths;
    cout << "Prune dominated equivalence classes? (1 = yes, 0 = no): ";
    cin >> config.pruneDominatedClasses;

    cout << endl;
    return config;
}

// Prune dominated equivalence classes: remove classes that are strictly dominated by another
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

// Helper function to run the solver for a given config
void runSolver(const Config &config)
{
    // --- Load Dictionary ---
    std::cout << "Reading word file...\n\n";
    std::string file_path_str = __FILE__;
    std::filesystem::path current_dir = std::filesystem::path(file_path_str).parent_path();
    std::ifstream file(current_dir / "words.txt");

    std::vector<std::string> allDictionaryWords;
    std::string line;
    if (file.is_open())
    {
        while (std::getline(file, line))
        {
            allDictionaryWords.push_back(stringToLower(line));
        }
        file.close();
        std::cout << "Loaded " << allDictionaryWords.size() << " words from dictionary.\n\n";
    }
    else
    {
        std::cerr << "Error: Could not open words.txt. Please ensure it's in the same directory as the executable.\n";
        std::cout << "\nPress any key to exit.\n";
        _getch();
        return;
    }

    // --- Setup PuzzleData from Config ---
    PuzzleData puzzleData;
    puzzleData.allLetters = config.letters;

    // Sides: 0=Top, 1=Left, 2=Right, 3=Bottom
    for (int i = 0; i < 3; ++i)
        puzzleData.letterToSideMapping[i] = 0; // Top
    for (int i = 3; i < 6; ++i)
        puzzleData.letterToSideMapping[i] = 1; // Left
    for (int i = 6; i < 9; ++i)
        puzzleData.letterToSideMapping[i] = 2; // Right
    for (int i = 9; i < 12; ++i)
        puzzleData.letterToSideMapping[i] = 3; // Bottom

    for (int i = 0; i < 12; ++i)
    {
        puzzleData.uniquePuzzleLetters.set(i);
    }

    // --- Start Solver ---
    cout << "Configuration set. Starting solver..." << endl;
    profiler.start();

    // --- STAGE 1: Filter dictionary and create all raw WordPath objects ---
    std::cout << "Filtering dictionary for valid words...\n";
    std::vector<int> allPathIndices; // Master vector for all path indices
    std::vector<WordPath> allValidWordPaths = filterWords(allDictionaryWords, puzzleData, config.minWordLength, config.minUniqueLetters, allPathIndices);
    std::cout << allValidWordPaths.size() << " valid word path(s) found.\n\n";

    // --- STAGE 2: Group WordPaths into Equivalence Classes ---
    std::cout << "Building equivalence classes...\n";
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

    std::vector<EquivalenceClass> allEqClasses;
    allEqClasses.reserve(eqClassMap.size());
    for (auto &pair : eqClassMap)
    {
        pair.second.key = pair.first;
        allEqClasses.push_back(pair.second);
    }
    std::cout << allEqClasses.size() << " unique equivalence classes created.\n\n";

    // --- Prune dominated equivalence classes if enabled ---
    if (config.pruneDominatedClasses)
    {
        pruneDominatedClasses(allEqClasses);
        std::cout << allEqClasses.size() << " equivalence classes after pruning dominated classes.\n\n";
    }

    // --- STAGE 3: Find solutions using the equivalence class graph ---
    std::cout << "Searching for class-based solution paths...\n";
    std::sort(allEqClasses.begin(), allEqClasses.end(),
              [&](const EquivalenceClass &a, const EquivalenceClass &b)
              {
                  return puzzleData.allLetters[a.key.startIndex] < puzzleData.allLetters[b.key.startIndex];
              });

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
    std::cout << classSolutions.size() << " abstract solution path(s) found.\n\n";

    // --- STAGE 4: Expand class solutions into final string solutions ---
    std::cout << "Expanding solutions into word combinations...\n";
    std::vector<Solution> finalSolutions;
    for (const auto &classPath : classSolutions)
    {
        std::vector<const WordPath *> currentWordChain;
        expandAndStoreSolutions(classPath, currentWordChain, 0, finalSolutions, puzzleData, allPathIndices);
    }
    std::cout << "Expansion complete.\n\n";

    profiler.end();
    profiler.logProfilerData();

    // --- Sort, unique, and print final solutions ---
    std::sort(finalSolutions.begin(), finalSolutions.end(), [](const Solution &a, const Solution &b)
              {
        if (a.wordCount != b.wordCount) return a.wordCount > b.wordCount;
        return a.text < b.text; });

    finalSolutions.erase(std::unique(finalSolutions.begin(), finalSolutions.end(), [](const Solution &a, const Solution &b)
                                     { return a.text == b.text; }),
                         finalSolutions.end());

    for (const auto &s : finalSolutions)
    {
        std::cout << s.text << "\n";
    }
    if (!finalSolutions.empty())
    {
        std::cout << "\n";
    }

    std::cout << finalSolutions.size() << " total solution(s) found.\n";
}

int main()
{
    while (true)
    {
        // --- STAGE 0: Get Configuration from User ---
        Config config = getUserConfiguration();

        runSolver(config);

        std::cout << "\nPress 'q' to quit, or any other key to run again.\n";
        char ch = _getch();
        if (ch == 'q' || ch == 'Q')
            break;
    }
    return 0;
}