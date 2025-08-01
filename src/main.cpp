#include "letterBoxed.hpp" // Your solver logic header
#include "utils.hpp"       // Your utilities header
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <cctype>
#include <filesystem>
#include <conio.h>
#include <algorithm>
#include <set>
#include <sstream>
#include <vector>
#include <filesystem>

/**
 * @brief Trim leading/trailing whitespace and convert to lowercase.
 */
std::string trimToLower(const std::string &s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    std::string out = s.substr(start, end - start + 1);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });
    return out;
}

// --- UI Drawing and Input Functions ---

/**
 * @brief Draws the puzzle box with the current letters.
 * @param letters The array of letters to display.
 */
void drawPuzzleBox(const std::array<char, 12> &letters)
{
    auto up = [](char c)
    { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); };

    std::cout << std::endl;
    std::cout << "      " << up(letters[0]) << " " << up(letters[1]) << " " << up(letters[2]) << std::endl;
    std::cout << "    +-------+" << std::endl;
    std::cout << "  " << up(letters[11]) << " |       | " << up(letters[3]) << std::endl;
    std::cout << "  " << up(letters[10]) << " |       | " << up(letters[4]) << std::endl;
    std::cout << "  " << up(letters[9]) << " |       | " << up(letters[5]) << std::endl;
    std::cout << "    +-------+" << std::endl;
    std::cout << "      " << up(letters[8]) << " " << up(letters[7]) << " " << up(letters[6]) << std::endl
              << std::endl;
}

/**
 * @brief Gathers all puzzle settings from the user via a single-line input process.
 * @return A Config struct populated with the user's settings.
 */
Config getUserConfiguration()
{
    Config config;
    config.letters.fill('*');

    // --- Step 1: Get the 12 puzzle letters, allowing spaces or no spaces ---
    while (true)
    {
        std::cout << "\nEnter the 12 puzzle letters (ex. abc def ghi jkl):" << std::endl;
        std::string input;
        std::getline(std::cin, input);

        // Remove all whitespace
        input.erase(std::remove_if(input.begin(), input.end(), ::isspace), input.end());

        if (input.size() != 12)
        {
            std::cout << "Invalid input. Please enter exactly 12 letters." << std::endl;
            continue;
        }

        bool valid = true;
        for (size_t i = 0; i < 12; ++i)
        {
            if (!isalpha(static_cast<unsigned char>(input[i])))
            {
                valid = false;
                break;
            }
            config.letters[i] = std::tolower(static_cast<unsigned char>(input[i]));
        }
        if (!valid)
        {
            std::cout << "All characters must be letters." << std::endl;
            continue;
        }
        break;
    }

    drawPuzzleBox(config.letters);

    // Helper lambda for validated integer input
    auto promptInt = [](const std::string &prompt, int def, int min, int max = -1)
    {
        while (true)
        {
            std::cout << prompt << " (default: " << def << "): ";
            std::string input;
            std::getline(std::cin, input);
            input = trimToLower(input);
            if (input.empty())
                return def;
            try
            {
                int val = std::stoi(input);
                if (val < min || (max > 0 && val > max))
                {
                    std::cout << "Value must be at least " << min;
                    if (max > 0)
                        std::cout << " and at most " << max;
                    std::cout << ".\n";
                    continue;
                }
                return val;
            }
            catch (...)
            {
                std::cout << "Invalid number. Try again.\n";
            }
        }
    };

    auto promptBool01 = [](const std::string &prompt, int def)
    {
        while (true)
        {
            std::cout << prompt << " (default: " << def << "): ";
            std::string input;
            std::getline(std::cin, input);
            input = trimToLower(input);
            if (input.empty())
                return def != 0;
            if (input == "0" || input == "n" || input == "no" || input == "f" || input == "false")
                return false;
            if (input == "1" || input == "y" || input == "yes" || input == "t" || input == "true")
                return true;
            std::cout << "Please enter 0 or 1.\n";
        }
    };

    // --- Step 2: Preset selection ---
    std::cout << "Select solver preset:\n"
              << "  1: Default (Will find ALL solutions up to 2 words)\n"
              << "  2: Fast (Will find most solutions up to 2 words quickly)\n"
              << "  3: Thorough (Will find ALL solutions up to 3 words)\n"
              << "  0: Custom (Configure manually)\n";
    int preset = promptInt("Enter preset number or blank for Default: ", 1, 0, 3);

    if (preset == 1)
    {
        // Default
        config.maxDepth = 2;
        config.minWordLength = 3;
        config.minUniqueLetters = 2;
        config.pruneRedundantPaths = true;
        config.pruneDominatedClasses = false;
        return config;
    }
    else if (preset == 2)
    {
        // Fast: less thorough, faster
        config.maxDepth = 2;
        config.minWordLength = 4;
        config.minUniqueLetters = 3;
        config.pruneRedundantPaths = true;
        config.pruneDominatedClasses = true;
        return config;
    }
    else if (preset == 3)
    {
        // Thorough: more exhaustive, slower
        config.maxDepth = 3;
        config.minWordLength = 3;
        config.minUniqueLetters = 1;
        config.pruneRedundantPaths = false;
        config.pruneDominatedClasses = false;
        return config;
    }

    // --- Step 3: Get solver options using std::cin ---
    std::cout << "Configure solver options. Press Enter to accept the default value." << std::endl
              << std::endl;

    config.maxDepth = promptInt("Max words per solutions", 2, 1, 4);
    config.minWordLength = promptInt("Min word length", 3, 1);
    config.minUniqueLetters = promptInt("Min unique letters per word", 2, 1);
    config.pruneRedundantPaths = promptBool01("Prune redundant paths?", 1);
    config.pruneDominatedClasses = promptBool01("Prune dominated classes?", 0);
    return config;
}

/**
 * @brief Main entry point for the application.
 */
int main()
{
    Utils::Profiler profiler;

    // Try to load wordVec from binary file
    std::vector<Word> wordVec;
    std::filesystem::path data_dir = std::filesystem::current_path() / "data";
    std::ifstream in(data_dir / "words.bin", std::ios::binary);
    bool loadedFromBin = false;
    if (in)
    {
        try
        {
            size_t n;
            in.read(reinterpret_cast<char *>(&n), sizeof(n));
            wordVec.resize(n);
            for (size_t i = 0; i < n; ++i)
            {
                size_t len;
                in.read(reinterpret_cast<char *>(&len), sizeof(len));
                wordVec[i].word.resize(len);
                in.read(&wordVec[i].word[0], len);
                in.read(reinterpret_cast<char *>(&wordVec[i].order), sizeof(wordVec[i].order));
                in.read(reinterpret_cast<char *>(&wordVec[i].count), sizeof(wordVec[i].count));
                if (!in)
                    throw std::runtime_error("Read error");
            }
            loadedFromBin = true;
            in.close();
            std::cout << "Loaded " << wordVec.size() << " words from words.bin\n";
        }
        catch (...)
        {
            std::cout << "Error loading words.bin, will rebuild from .txt files.\n";
            in.close();
            wordVec.clear();
        }
    }

    if (!loadedFromBin)
    {
        // Automatically find all .txt files in data directory and sort alphabetically
        std::vector<std::string> wordFiles;
        for (const auto &entry : std::filesystem::directory_iterator(data_dir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".txt")
            {
                wordFiles.push_back(entry.path().filename().string());
            }
        }
        std::sort(wordFiles.begin(), wordFiles.end());

        std::cout << "Loading words from " << wordFiles.size() << " files..." << std::endl;

        std::set<Word> allWords;
        int order = 0;

        for (const auto &fname : wordFiles)
        {
            std::ifstream file(data_dir / fname);
            if (!file.is_open())
            {
                std::cerr << "Error: Could not open " << fname << ". Please ensure it's in a 'data' sub-directory.\n";
                continue;
            }
            std::string line;
            while (std::getline(file, line))
            {
                std::istringstream iss(line);
                std::string word;
                while (iss >> word)
                {
                    word = trimToLower(word);

                    if (word.empty() || std::any_of(word.begin(), word.end(), [](unsigned char c)
                                                    { return !std::isalpha(c); }))
                    {
                        std::cout << "Ignoring invalid word: " << word << std::endl;
                        continue;
                    }

                    auto result = allWords.insert({word, order, 1});
                    if (!result.second)
                    {
                        auto it = result.first;
                        Word updatedWord = *it;
                        allWords.erase(it);
                        updatedWord.count += 1;
                        allWords.insert(updatedWord);
                    }
                }
            }
            file.close();
            order++;
        }
        std::cout << "Loaded " << allWords.size() << " unique words from all files.\n"
                  << std::endl;
        wordVec.assign(allWords.begin(), allWords.end());

        // Save to binary for next time
        std::ofstream out(data_dir / "words.bin", std::ios::binary);
        size_t n = wordVec.size();
        out.write(reinterpret_cast<const char *>(&n), sizeof(n));
        for (const auto &w : wordVec)
        {
            size_t len = w.word.size();
            out.write(reinterpret_cast<const char *>(&len), sizeof(len));
            out.write(w.word.data(), len);
            out.write(reinterpret_cast<const char *>(&w.order), sizeof(w.order));
            out.write(reinterpret_cast<const char *>(&w.count), sizeof(w.count));
        }
        out.close();
        std::cout << "Saved " << wordVec.size() << " words to words.bin\n";
    }

    int totalLetterCount = 0;
    for (const auto &word : wordVec)
    {
        totalLetterCount += word.word.size();
    }

    while (true)
    {
        Config config = getUserConfiguration();
        std::cout << "\nSolver configuration:\n";
        std::cout << "  Max words per solution: " << config.maxDepth << "\n";
        std::cout << "  Min word length: " << config.minWordLength << "\n";
        std::cout << "  Min unique letters per word: " << config.minUniqueLetters << "\n";
        std::cout << "  Prune redundant paths: " << (config.pruneRedundantPaths ? "true" : "false") << "\n";
        std::cout << "  Prune dominated classes: " << (config.pruneDominatedClasses ? "true" : "false") << "\n\n";

        std::cout << "Running solver...\n\n";

        PuzzleData puzzleData;
        puzzleData.allLetters = config.letters;
        for (int i = 0; i < 3; ++i)
            puzzleData.letterToSideMapping[i] = 0;
        for (int i = 3; i < 6; ++i)
            puzzleData.letterToSideMapping[i] = 1;
        for (int i = 6; i < 9; ++i)
            puzzleData.letterToSideMapping[i] = 2;
        for (int i = 9; i < 12; ++i)
            puzzleData.letterToSideMapping[i] = 3;
        for (int i = 0; i < 12; ++i)
            puzzleData.uniquePuzzleLetters.set(i);
        puzzleData.charToIndexMap.fill(-1); // Initialize all to -1
        for (int i = 0; i < 12; ++i)
        {
            puzzleData.charToIndexMap[static_cast<unsigned char>(puzzleData.allLetters[i])] = i;
        }

        std::vector<Solution> finalSolutions;

        profiler.start();
        runLetterBoxedSolver(
            puzzleData,
            wordVec,
            totalLetterCount,
            config,
            finalSolutions);
        profiler.end();
        profiler.logProfilerData();

        // Print only top 100 solutions, if 'a' is entered reprint all
        int printLimit = 100;
        auto printSolutions = [&](int limit)
        {
            int toPrint = std::min(limit, static_cast<int>(finalSolutions.size()));
            for (int i = toPrint - 1; i >= 0; --i)
            {
                const Solution &sol = finalSolutions[i];
                std::cout << sol.text
                          //   << " (Count: " << sol.countMin << ", " << sol.countSum << ", " << sol.countMax << ")"
                          //   << " (Order: " << sol.orderMin << ", " << sol.orderSum << ", " << sol.orderMax << ")"
                          << "\n";
            }
            std::cout << "\nFound " + std::to_string(finalSolutions.size()) + " final solutions in " + std::to_string(profiler.getTotalTime()) + " seconds.\n";
            if (limit < static_cast<int>(finalSolutions.size()))
                std::cout << "Showing top " << toPrint << " of " << finalSolutions.size() << " solution(s).\n\n";
            else
                std::cout << "Showing all " << finalSolutions.size() << " solution(s).\n\n";
        };

        printSolutions(printLimit);

        while (true)
        {
            std::cout << "Enter 'q' to quit, 'r' to restart, or 'a' to show all.\n\n";
            std::string input;
            std::getline(std::cin, input);
            input = trimToLower(input);
            if (!input.empty())
            {
                if (input == "q")
                {
                    return 0;
                }
                else if (input == "r")
                {
                    break; // restart outer loop
                }
                else if (input == "a")
                {
                    std::cout << "\n";
                    printSolutions(static_cast<int>(finalSolutions.size()));
                }
            }
        }
    }
    return 0;
}
