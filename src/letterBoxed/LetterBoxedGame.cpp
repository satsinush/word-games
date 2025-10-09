#include <iostream>
#include <string>
#include <algorithm>
#include <set>
#include <filesystem>
#include <fstream>

#include "LetterBoxedGame.hpp"
#include "../utils/inputUtils.hpp"

namespace Game
{
    LetterBoxedGame::LetterBoxedGame(const std::vector<Utils::Word> &words)
        : wordVec(words)
    {
        totalLetterCount = 0;
        for (const auto &word : wordVec)
            totalLetterCount += word.wordString.size();
    }

    void LetterBoxedGame::drawPuzzle(const std::array<char, 12> &letters)
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

    LetterBoxed::Config LetterBoxedGame::getConfigFromUser()
    {
        LetterBoxed::Config config;
        config.allLetters.fill('*');

        // Get puzzle letters
        std::string letters = Utils::Input::promptLetters("Enter the 12 puzzle letters (ex. abc def ghi jkl):", 12, false);

        for (size_t i = 0; i < 12; ++i)
        {
            config.allLetters[i] = letters[i];
        }

        // Set up letter mappings
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

        config.charToIndexMap.fill(-1);
        for (int i = 0; i < 12; ++i)
        {
            config.charToIndexMap[static_cast<unsigned char>(config.allLetters[i])] = i;
        }

        drawPuzzle(config.allLetters);

        // Preset selection
        std::cout << "Select solver preset:\n"
                  << "  1: Default (Will find ALL solutions up to 2 words)\n"
                  << "  2: Fast (Will find most solutions up to 2 words quickly)\n"
                  << "  3: Thorough (Will find ALL solutions up to 3 words)\n"
                  << "  0: Custom (Configure manually)\n";

        int preset = Utils::Input::promptInt("Enter preset number", 1, 0, 3);

        if (preset == 1)
        {
            // Default
            config.maxDepth = 2;
            config.minWordLength = 3;
            config.minUniqueLetters = 2;
            config.pruneRedundantPaths = true;
            config.pruneDominatedClasses = false;
        }
        else if (preset == 2)
        {
            // Fast
            config.maxDepth = 2;
            config.minWordLength = 4;
            config.minUniqueLetters = 3;
            config.pruneRedundantPaths = true;
            config.pruneDominatedClasses = true;
        }
        else if (preset == 3)
        {
            // Thorough
            config.maxDepth = 3;
            config.minWordLength = 3;
            config.minUniqueLetters = 2;
            config.pruneRedundantPaths = true;
            config.pruneDominatedClasses = false;
        }
        else
        {
            // Custom configuration
            std::cout << "Configure solver options:" << std::endl;
            config.maxDepth = Utils::Input::promptInt("Max words per solution", 2, 1, 4);
            config.minWordLength = Utils::Input::promptInt("Min word length", 3, 1);
            config.minUniqueLetters = Utils::Input::promptInt("Min unique letters per word", 2, 1);
            config.pruneRedundantPaths = Utils::Input::promptBool("Prune redundant paths?", true);
            config.pruneDominatedClasses = Utils::Input::promptBool("Prune dominated classes?", false);
        }

        return config;
    }

    LetterBoxed::Config LetterBoxedGame::getConfigFromArgs(const std::map<std::string, std::string> &args)
    {
        LetterBoxed::Config config;
        config.allLetters.fill('*');

        // Parse letters
        std::string letters = Utils::Input::getArgValue(args, "letters", std::string(""));
        if (letters.empty() || letters.size() != 12)
        {
            throw std::invalid_argument("Invalid letters argument. Must provide exactly 12 letters.");
        }

        letters.erase(std::remove_if(letters.begin(), letters.end(), ::isspace), letters.end());
        std::transform(letters.begin(), letters.end(), letters.begin(), ::tolower);

        for (size_t i = 0; i < 12; ++i)
        {
            if (!isalpha(static_cast<unsigned char>(letters[i])))
                throw std::invalid_argument("All characters must be letters.");
            config.allLetters[i] = letters[i];
        }

        // Set up mappings
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

        config.charToIndexMap.fill(-1);
        for (int i = 0; i < 12; ++i)
        {
            config.charToIndexMap[static_cast<unsigned char>(config.allLetters[i])] = i;
        }

        // Parse preset or custom settings
        int preset = Utils::Input::getArgValue(args, "preset", 1);

        if (preset == 1)
        {
            config.maxDepth = 2;
            config.minWordLength = 3;
            config.minUniqueLetters = 2;
            config.pruneRedundantPaths = true;
            config.pruneDominatedClasses = false;
        }
        else if (preset == 2)
        {
            config.maxDepth = 2;
            config.minWordLength = 4;
            config.minUniqueLetters = 3;
            config.pruneRedundantPaths = true;
            config.pruneDominatedClasses = true;
        }
        else if (preset == 3)
        {
            config.maxDepth = 3;
            config.minWordLength = 3;
            config.minUniqueLetters = 2;
            config.pruneRedundantPaths = true;
            config.pruneDominatedClasses = false;
        }
        else if (preset == 0)
        {
            // Custom - read individual settings
            config.maxDepth = Utils::Input::getArgValue(args, "maxDepth", 2);
            config.minWordLength = Utils::Input::getArgValue(args, "minWordLength", 3);
            config.minUniqueLetters = Utils::Input::getArgValue(args, "minUniqueLetters", 2);
            config.pruneRedundantPaths = Utils::Input::getArgValue(args, "pruneRedundantPaths", true);
            config.pruneDominatedClasses = Utils::Input::getArgValue(args, "pruneDominatedClasses", false);
        }

        return config;
    }

    void LetterBoxedGame::printSolutions(const std::vector<LetterBoxed::Solution> &solutions, int limit)
    {
        int lastNumWords = 0;
        int toPrint = std::min(limit, static_cast<int>(solutions.size()));

        for (int i = toPrint - 1; i >= 0; --i)
        {
            if (lastNumWords == 0 || (solutions[i].wordCount != lastNumWords))
            {
                std::cout << "\n=== " << solutions[i].wordCount << " word solutions ===\n";
            }
            std::cout << solutions[i].text << "\n";
            lastNumWords = solutions[i].wordCount;
        }

        std::cout << "\nFound " << solutions.size() << " final solutions.";
        if (limit < static_cast<int>(solutions.size()))
            std::cout << " (Showing top " << limit << ")";
        std::cout << "\n";
    }

    void LetterBoxedGame::runCLI()
    {
        while (true)
        {
            LetterBoxed::Config config = getConfigFromUser();

            std::cout << "\nSolver configuration:\n";
            std::cout << "  Max words per solution: " << config.maxDepth << "\n";
            std::cout << "  Min word length: " << config.minWordLength << "\n";
            std::cout << "  Min unique letters per word: " << config.minUniqueLetters << "\n";
            std::cout << "  Prune redundant paths: " << (config.pruneRedundantPaths ? "true" : "false") << "\n";
            std::cout << "  Prune dominated classes: " << (config.pruneDominatedClasses ? "true" : "false") << "\n\n";

            std::cout << "Running solver...\n";
            std::vector<LetterBoxed::Solution> solutions = LetterBoxed::runLetterBoxedSolver(config, wordVec, totalLetterCount);

            int printLimit = 100;
            printSolutions(solutions, printLimit);

            while (true)
            {
                std::cout << "Enter 'q' to quit, 'r' to restart, or 'a' to show all.\n\n";
                std::string input;
                std::getline(std::cin, input);
                input = Utils::trimToLower(input);

                if (input == "q")
                    return;
                if (input == "r")
                    break;
                if (input == "a")
                {
                    printSolutions(solutions, static_cast<int>(solutions.size()));
                    continue;
                }
            }
        }
    }

    void LetterBoxedGame::runHeadless(const std::map<std::string, std::string> &args)
    {
        try
        {
            LetterBoxed::Config config = getConfigFromArgs(args);

            std::vector<LetterBoxed::Solution> solutions = LetterBoxed::runLetterBoxedSolver(config, wordVec, totalLetterCount);

            // Output to file if specified
            std::string outputFile = Utils::Input::getArgValue(args, "file", std::string("results/temp.txt"));

            // Ensure directory exists
            std::filesystem::path filePath(outputFile);
            if (!filePath.parent_path().empty() && !std::filesystem::exists(filePath.parent_path()))
            {
                std::filesystem::create_directories(filePath.parent_path());
            }

            std::ofstream out(outputFile);
            if (out.is_open())
            {
                for (const auto &solution : solutions)
                {
                    out << solution.text << "\n";
                }
                out.close();
            }
            else
            {
                std::cerr << "Could not write to file: " << outputFile << "\n";
            }

            std::cout << solutions.size() << "\n";
            std::cout << outputFile;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }

    void LetterBoxedGame::runGUI()
    {
        std::cout << "GUI mode not yet implemented for Letter Boxed.\n";
    }
}