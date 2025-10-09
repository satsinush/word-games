#include <iostream>
#include <string>
#include <algorithm>
#include <set>
#include <filesystem>
#include <fstream>

#include "SpellingBeeGame.hpp"
#include "../utils/inputUtils.hpp"

namespace Game
{
    SpellingBeeGame::SpellingBeeGame(const std::vector<Utils::Word> &words)
        : wordVec(words)
    {
    }

    void SpellingBeeGame::drawPuzzle(const std::array<char, 7> &letters)
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

    SpellingBee::Config SpellingBeeGame::getConfigFromUser()
    {
        SpellingBee::Config config;

        // Get puzzle letters (7 unique letters)
        std::string letters = Utils::Input::promptLetters("Enter the 7 puzzle letters (ex. a bcdefg):", 7, false);

        for (size_t i = 0; i < 7; ++i)
        {
            config.allLetters[i] = letters[i];
        }

        // Set up valid letters map
        for (char c : config.allLetters)
        {
            config.validLettersMap[static_cast<unsigned char>(c)] = true;
        }

        drawPuzzle(config.allLetters);

        return config;
    }

    SpellingBee::Config SpellingBeeGame::getConfigFromArgs(const std::map<std::string, std::string> &args)
    {
        SpellingBee::Config config;

        // Parse letters
        std::string letters = Utils::Input::getArgValue(args, "letters", std::string(""));
        if (letters.empty())
        {
            throw std::invalid_argument("Missing required argument: letters");
        }

        letters.erase(std::remove_if(letters.begin(), letters.end(), ::isspace), letters.end());
        std::transform(letters.begin(), letters.end(), letters.begin(), ::tolower);

        if (letters.size() != 7)
        {
            throw std::invalid_argument("Must provide exactly 7 letters for Spelling Bee.");
        }

        // Check for duplicates and validate letters
        std::set<char> seen;
        for (size_t i = 0; i < 7; ++i)
        {
            char c = letters[i];
            if (!isalpha(static_cast<unsigned char>(c)))
                throw std::invalid_argument("All characters must be letters.");
            if (seen.count(c))
                throw std::invalid_argument("Duplicate letters not allowed in Spelling Bee.");
            seen.insert(c);
            config.allLetters[i] = c;
        }

        // Set up valid letters map
        for (char c : config.allLetters)
        {
            config.validLettersMap[static_cast<unsigned char>(c)] = true;
        }

        return config;
    }

    void SpellingBeeGame::printSolutions(const std::vector<Utils::Word> &solutions)
    {
        int lastUniqueLetters = 0;
        for (auto it = solutions.rbegin(); it != solutions.rend(); ++it)
        {
            if (lastUniqueLetters == 0 || (it->uniqueLetters != lastUniqueLetters))
            {
                if (lastUniqueLetters != 0)
                    std::cout << "\n";
                std::cout << "=== " << it->uniqueLetters << " unique letters ===\n";
            }
            std::cout << it->wordString << "\n";
            lastUniqueLetters = it->uniqueLetters;
        }
        if (solutions.size() > 0)
            std::cout << "\n";
        std::cout << solutions.size() << " valid word(s) found.\n";
    }

    void SpellingBeeGame::runCLI()
    {
        while (true)
        {
            SpellingBee::Config config = getConfigFromUser();

            std::cout << "Running solver...\n";
            std::vector<Utils::Word> solutions = SpellingBee::runSpellingBeeSolver(wordVec, config);

            printSolutions(solutions);

            while (true)
            {
                std::cout << "Enter 'q' to quit, 'r' to restart.\n\n";
                std::string input;
                std::getline(std::cin, input);
                input = Utils::trimToLower(input);

                if (input == "q")
                    return;
                if (input == "r")
                    break;
            }
        }
    }

    void SpellingBeeGame::runHeadless(const std::map<std::string, std::string> &args)
    {
        try
        {
            SpellingBee::Config config = getConfigFromArgs(args);

            std::vector<Utils::Word> solutions = SpellingBee::runSpellingBeeSolver(wordVec, config);

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
                for (const auto &word : solutions)
                {
                    out << word.wordString << "\n";
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

    void SpellingBeeGame::runGUI()
    {
        std::cout << "GUI mode not yet implemented for Spelling Bee.\n";
    }
}