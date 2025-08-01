#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <set>
#include <sstream>
#include <filesystem>

#include "utils.hpp"
#include "letterBoxed.hpp"
#include "spellingBee.hpp"

// --- Letter Boxed UI and Game Loop ---
void drawLetterBoxedPuzzle(const std::array<char, 12> &letters)
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

LetterBoxed::Config getLetterBoxedConfig()
{
    LetterBoxed::Config config;
    config.allLetters.fill('*');
    config.allLetters.fill('*');

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
            config.allLetters[i] = std::tolower(static_cast<unsigned char>(input[i]));
        }
        if (!valid)
        {
            std::cout << "All characters must be letters." << std::endl;
            continue;
        }
        break;
    }

    for (int i = 0; i < 3; ++i)
        config.letterToSideMapping[i] = 0;
    for (int i = 3; i < 6; ++i)
        config.letterToSideMapping[i] = 1;
    for (int i = 6; i < 9; ++i)
        config.letterToSideMapping[i] = 2;
    for (int i = 9; i < 12; ++i)
        config.letterToSideMapping[i] = 3;
    for (int i = 0; i < 12; ++i)
        config.uniquePuzzleLetters.set(i);
    config.charToIndexMap.fill(-1); // Initialize all to -1
    for (int i = 0; i < 12; ++i)
    {
        config.charToIndexMap[static_cast<unsigned char>(config.allLetters[i])] = i;
    }

    drawLetterBoxedPuzzle(config.allLetters);

    // Helper lambda for validated integer input
    auto promptInt = [](const std::string &prompt, int def, int min, int max = -1)
    {
        while (true)
        {
            std::cout << prompt << " (default: " << def << "): ";
            std::string input;
            std::getline(std::cin, input);
            input = WordUtils::trimToLower(input);
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
            input = WordUtils::trimToLower(input);
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

void runLetterBoxedGame(const std::vector<WordUtils::Word> &wordVec, bool logData = false)
{
    ProfilerUtils::Profiler profiler;
    int totalLetterCount = 0;
    for (const auto &word : wordVec)
        totalLetterCount += word.wordString.size();

    while (true)
    {
        LetterBoxed::Config config = getLetterBoxedConfig();
        std::cout << "\nSolver configuration:\n";
        std::cout << "  Max words per solution: " << config.maxDepth << "\n";
        std::cout << "  Min word length: " << config.minWordLength << "\n";
        std::cout << "  Min unique letters per word: " << config.minUniqueLetters << "\n";
        std::cout << "  Prune redundant paths: " << (config.pruneRedundantPaths ? "true" : "false") << "\n";
        std::cout << "  Prune dominated classes: " << (config.pruneDominatedClasses ? "true" : "false") << "\n\n";

        std::cout << "Running solver...\n";
        profiler.start();
        std::vector<LetterBoxed::Solution> finalSolutions = LetterBoxed::runLetterBoxedSolver(config, wordVec, totalLetterCount);
        profiler.end();
        if (logData)
            profiler.logProfilerData();

        int printLimit = 100;
        auto printSolutions = [&](int limit)
        {
            int lastNumWords = 0;
            int toPrint = std::min(limit, static_cast<int>(finalSolutions.size()));
            for (int i = toPrint - 1; i >= 0; --i)
            {
                const auto &sol = finalSolutions[i];
                if (lastNumWords == 0 || sol.wordCount != lastNumWords)
                {
                    std::cout << "\n";
                    std::cout << "  -- " << sol.wordCount << " word solutions --\n";
                }
                std::cout << sol.text << "\n";
                lastNumWords = sol.wordCount;
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
            input = WordUtils::trimToLower(input);
            if (!input.empty())
            {
                if (input == "q")
                    return;
                else if (input == "r")
                    break;
                else if (input == "a")
                {
                    std::cout << "\n";
                    printSolutions(static_cast<int>(finalSolutions.size()));
                }
            }
        }
    }
}

// --- Spelling Bee UI and Game Loop ---
void drawSpellingBeePuzzle(const std::array<char, 7> &letters)
{
    auto up = [](char c)
    { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); };
    std::cout << std::endl;
    std::cout << "      " << up(letters[1]) << std::endl;
    std::cout << "   " << up(letters[6]) << "     " << up(letters[2]) << std::endl;
    std::cout << "      " << up(letters[0]) << std::endl;
    std::cout << "   " << up(letters[5]) << "     " << up(letters[3]) << std::endl;
    std::cout << "      " << up(letters[4]) << std::endl
              << std::endl;
}

SpellingBee::Config getSpellingBeeConfig()
{
    SpellingBee::Config config;

    // --- Step 1: Get the 7 puzzle letters, allowing spaces or no spaces ---
    std::string input;
    while (true)
    {
        std::cout << "\nEnter the 7 puzzle letters (ex. a bcdefg):" << std::endl;
        std::getline(std::cin, input);

        // Remove all whitespace
        input.erase(std::remove_if(input.begin(), input.end(), ::isspace), input.end());

        if (input.size() != 7)
        {
            std::cout << "Invalid input. Please enter exactly 7 letters." << std::endl;
            continue;
        }

        bool valid = true;
        std::set<char> seen;
        for (size_t i = 0; i < 7; ++i)
        {
            char c = static_cast<char>(std::tolower(static_cast<unsigned char>(input[i])));
            if (!isalpha(static_cast<unsigned char>(input[i])))
            {
                valid = false;
                std::cout << "Invalid character '" << input[i] << "'. Only letters are allowed." << std::endl;
                break;
            }
            if (seen.count(c))
            {
                valid = false;
                std::cout << "All letters must be different." << std::endl;
                break;
            }
            seen.insert(c);
            config.allLetters[i] = c;
        }
        if (!valid)
        {
            continue;
        }
        break;
    }

    for (char c : config.allLetters)
    {
        config.validLettersMap[static_cast<unsigned char>(c)] = true;
    }

    drawSpellingBeePuzzle(config.allLetters);

    return config;
}

void runSpellingBeeGame(const std::vector<WordUtils::Word> &allWordsVec, bool logData = false)
{
    ProfilerUtils::Profiler profiler;
    while (true)
    {
        SpellingBee::Config config = getSpellingBeeConfig();
        std::cout << "Running solver...\n";
        profiler.start();
        std::vector<WordUtils::Word> solutions = SpellingBee::runSpellingBeeSolver(allWordsVec, config);
        profiler.end();
        if (logData)
            profiler.logProfilerData();

        int lastUniqueLetters = 0;
        for (auto it = solutions.rbegin(); it != solutions.rend(); ++it)
        {
            if (lastUniqueLetters == 0 || (it->uniqueLetters != lastUniqueLetters))
            {
                std::cout << "\n";
                std::cout << "  -- " << it->uniqueLetters << " unique letters";
                if (it->uniqueLetters == 7)
                    std::cout << " (PANGRAMS!)";
                std::cout << " --\n";
            }
            std::cout << it->wordString << "\n";
            lastUniqueLetters = it->uniqueLetters;
        }
        if (solutions.size() > 0)
            std::cout << "\n";
        std::cout << solutions.size() << " valid word(s) found in " << profiler.getTotalTime() << " seconds.\n";

        while (true)
        {
            std::cout << "Enter 'q' to quit, 'r' to restart.\n\n";
            std::string input;
            std::getline(std::cin, input);
            input = WordUtils::trimToLower(input);
            if (!input.empty())
            {
                if (input == "q")
                    return;
                else if (input == "r")
                    break;
            }
        }
    }
}

// --- Combined Main Loop ---
int main()
{
    std::vector<WordUtils::Word> allWordsVec = WordUtils::loadWords();
    bool logData = false;

    while (true)
    {
        std::cout << "\nSelect game mode:\n";
        std::cout << "  1: Letter Boxed\n";
        std::cout << "  2: Spelling Bee\n";
        std::cout << "  q: Quit\n";
        std::cout << "Enter choice: ";
        std::string input;
        std::getline(std::cin, input);
        if (input.empty())
            continue;
        if (input == "q" || input == "Q")
            break;
        if (input == "1")
            runLetterBoxedGame(allWordsVec, logData);
        else if (input == "2")
            runSpellingBeeGame(allWordsVec, logData);
        else
            std::cout << "Invalid choice. Try again.\n";
    }
    std::cout << "Goodbye!\n";
    return 0;
}
