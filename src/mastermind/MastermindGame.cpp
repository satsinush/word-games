#include <iostream>
#include <string>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <iomanip>

#include "MastermindGame.hpp"
#include "../utils/inputUtils.hpp"

namespace Game
{
    Mastermind::Config MastermindGame::getConfigFromUser()
    {
        Mastermind::Config config;

        std::cout << "=== Mastermind Solver ===\n";
        config.numPegs = Utils::Input::promptInt("Enter number of pegs", 4, 1, 20);
        config.numColors = Utils::Input::promptInt("Enter number of colors", 6, 1, 20);
        config.allowDuplicates = Utils::Input::promptBool("Allow duplicate colors?", true);

        return config;
    }

    Mastermind::Config MastermindGame::getConfigFromArgs(const std::map<std::string, std::string> &args)
    {
        Mastermind::Config config;
        config.numPegs = Utils::Input::getArgValue(args, "numPegs", 4u);
        config.numColors = Utils::Input::getArgValue(args, "numColors", 6u);
        config.allowDuplicates = Utils::Input::getArgValue(args, "allowDuplicates", true);
        config.maxDepth = Utils::Input::getArgValue(args, "maxDepth", 1u);
        return config;
    }

    std::vector<Mastermind::Feedback> MastermindGame::getFeedbackFromUser(const Mastermind::Config &config)
    {
        std::vector<Mastermind::Feedback> guessHistory;

        std::cout << "\nEnter your guesses and feedback.\n";
        std::cout << "Format: 1 2 3 4|2 1 (pattern|correct_position correct_color)\n";
        std::cout << "Enter 'done' when finished entering feedback.\n\n";

        while (true)
        {
            std::cout << "Enter guess and feedback (or 'done'): ";
            std::string input;
            std::getline(std::cin, input);

            if (Utils::trimToLower(input) == "done")
                break;
            if (input.empty())
                continue;

            try
            {
                // Split input by pipe separator
                size_t pipePos = input.find('|');
                if (pipePos == std::string::npos)
                {
                    std::cout << "Error: Missing '|' separator. Use format: 1 2 3 4|2 1\n";
                    continue;
                }

                std::string patternStr = input.substr(0, pipePos);
                std::string feedbackStr = input.substr(pipePos + 1);

                // Parse pattern colors
                std::istringstream patternIss(patternStr);
                std::vector<uint8_t> colors;
                std::string token;
                while (patternIss >> token)
                {
                    int color = std::stoi(token);
                    if (color < 0 || color > static_cast<int>(config.numColors))
                    {
                        throw std::invalid_argument("Color out of range");
                    }
                    colors.push_back(static_cast<uint8_t>(color));
                }

                if (colors.size() != config.numPegs)
                {
                    std::cout << "Error: Pattern must have exactly " << config.numPegs << " colors.\n";
                    continue;
                }

                // Parse feedback
                std::istringstream feedbackIss(feedbackStr);
                int correctPos, correctCol;
                if (!(feedbackIss >> correctPos >> correctCol))
                {
                    std::cout << "Error: Invalid feedback format. Use: correct_position correct_color\n";
                    continue;
                }

                if (correctPos > static_cast<int>(config.numPegs) || correctCol > static_cast<int>(config.numPegs) ||
                    correctPos < 0 || correctCol < 0 || (correctPos + correctCol) > static_cast<int>(config.numPegs))
                {
                    std::cout << "Error: Invalid feedback values.\n";
                    continue;
                }

                Mastermind::Pattern pattern(colors);
                Mastermind::Feedback feedback;
                feedback.guess = pattern;
                feedback.correctPosition = static_cast<uint8_t>(correctPos);
                feedback.correctColor = static_cast<uint8_t>(correctCol);

                guessHistory.push_back(feedback);
                std::cout << "Added: " << pattern.toString() << " with feedback "
                          << static_cast<int>(feedback.correctPosition) << " "
                          << static_cast<int>(feedback.correctColor) << "\n";
            }
            catch (const std::exception &e)
            {
                std::cout << "Error parsing input: " << e.what() << "\n";
                std::cout << "Please use format: 1 2 3 4|2 1\n";
            }
        }

        return guessHistory;
    }

    std::vector<Mastermind::Feedback> MastermindGame::getFeedbackFromArgs(const std::map<std::string, std::string> &args, const Mastermind::Config &config)
    {
        std::vector<Mastermind::Feedback> guessHistory;

        // Look for guesses argument
        auto it = args.find("guesses");
        if (it == args.end())
        {
            return guessHistory; // Return empty if no guesses provided
        }

        // Parse multiple feedback strings in format: "1 1 2 2|1 2;0 0 0 4|1 2"
        std::istringstream iss(it->second);
        std::string guessStr;

        // Parse each guess separated by semicolons
        while (std::getline(iss, guessStr, ';'))
        {
            std::string trimmed = Utils::trimToLower(guessStr);
            if (trimmed.empty() || trimmed.find('|') == std::string::npos)
                continue;

            try
            {
                // Split by pipe
                size_t pipePos = trimmed.find('|');
                std::string patternStr = trimmed.substr(0, pipePos);
                std::string feedbackStr = trimmed.substr(pipePos + 1);

                // Parse pattern
                std::istringstream patternIss(patternStr);
                std::vector<uint8_t> colors;
                std::string token;
                while (patternIss >> token)
                {
                    int color = std::stoi(token);
                    if (color < 0 || color > static_cast<int>(config.numColors))
                    {
                        throw std::invalid_argument("Color out of range");
                    }
                    colors.push_back(static_cast<uint8_t>(color));
                }

                if (colors.size() != config.numPegs)
                {
                    throw std::invalid_argument("Invalid number of pegs");
                }

                // Parse feedback
                std::istringstream feedbackIss(feedbackStr);
                int correctPos, correctCol;
                if (feedbackIss >> correctPos >> correctCol)
                {
                    Mastermind::Pattern pattern(colors);
                    Mastermind::Feedback feedback;
                    feedback.guess = pattern;
                    feedback.correctPosition = static_cast<uint8_t>(correctPos);
                    feedback.correctColor = static_cast<uint8_t>(correctCol);
                    guessHistory.push_back(feedback);
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Warning: Could not parse feedback '" << guessStr << "': " << e.what() << "\n";
            }
        }

        return guessHistory;
    }

    void MastermindGame::printResults(const Mastermind::Result &result)
    {
        std::cout << "\n=== SOLVER RESULTS ===\n";

        if (result.totalPossiblePatterns == 0)
        {
            std::cout << "No possible patterns found with given constraints.\n";
            return;
        }

        std::cout << "Possible patterns remaining: " << result.totalPossiblePatterns << "\n\n";

        if (!result.sortedGuesses.empty())
        {
            std::cout << "Best next guesses (top 10):\n";
            std::cout << std::setw(20) << "Pattern" << std::setw(12) << "Entropy" << std::setw(15) << "Probability" << "\n";
            std::cout << std::string(50, '-') << "\n";

            int count = 0;
            for (const auto &guess : result.sortedGuesses)
            {
                if (++count > 10)
                    break;

                std::cout << std::setw(20) << guess.pattern.toString()
                          << std::setw(12) << std::fixed << std::setprecision(3) << guess.entropy
                          << std::setw(15) << std::fixed << std::setprecision(6) << guess.probability << "\n";
            }
        }
    }

    void MastermindGame::saveResults(const Mastermind::Result &result, const std::string &possibleFile, const std::string &guessesFile)
    {
        // Save possible patterns
        std::filesystem::path possiblePath(possibleFile);
        if (!possiblePath.parent_path().empty() && !std::filesystem::exists(possiblePath.parent_path()))
        {
            std::filesystem::create_directories(possiblePath.parent_path());
        }

        std::ofstream possibleOut(possibleFile);
        if (possibleOut.is_open())
        {
            // Extract possible patterns from sorted guesses that have probability > 0
            for (const auto &guess : result.sortedGuesses)
            {
                if (guess.probability > 0.0)
                {
                    possibleOut << guess.pattern.toString() << "\n";
                }
            }
            possibleOut.close();
        }

        // Save all guesses with entropy
        std::filesystem::path guessesPath(guessesFile);
        if (!guessesPath.parent_path().empty() && !std::filesystem::exists(guessesPath.parent_path()))
        {
            std::filesystem::create_directories(guessesPath.parent_path());
        }

        std::ofstream guessesOut(guessesFile);
        if (guessesOut.is_open())
        {
            guessesOut << "pattern,entropy,probability\n";
            for (const auto &guess : result.sortedGuesses)
            {
                guessesOut << "\"" << guess.pattern.toString() << "\","
                           << guess.entropy << ","
                           << guess.probability << "\n";
            }
            guessesOut.close();
        }

        std::cout << result.totalPossiblePatterns << "\n";
        std::cout << result.sortedGuesses.size() << "\n";
        std::cout << possibleFile << "\n";
        std::cout << guessesFile;
    }

    void MastermindGame::runCLI()
    {
        Mastermind::Config config = getConfigFromUser();

        // Generate all possible patterns
        std::vector<Mastermind::Pattern> allPatterns = Mastermind::generateAllPatterns(config);
        std::cout << "Generated " << allPatterns.size() << " possible patterns.\n\n";

        std::vector<Mastermind::Feedback> guessHistory;

        while (true)
        {
            std::cout << "Current guess history:\n";
            for (size_t i = 0; i < guessHistory.size(); ++i)
            {
                std::cout << (i + 1) << ". " << guessHistory[i].guess.toString()
                          << " -> " << static_cast<int>(guessHistory[i].correctPosition)
                          << " " << static_cast<int>(guessHistory[i].correctColor) << "\n";
            }

            std::cout << "\nCommands:\n";
            std::cout << "  'solve' - Calculate best next guess\n";
            std::cout << "  'clear' - Clear guess history\n";
            std::cout << "  'q' - Quit\n";
            std::cout << "  Or enter: 'PATTERN|FEEDBACK' (e.g., '1 2 3 4|2 1')\n";
            std::cout << "\nEnter command: ";

            std::string input;
            std::getline(std::cin, input);

            if (input == "q")
                return;

            if (input == "clear")
            {
                guessHistory.clear();
                std::cout << "Guess history cleared.\n\n";
                continue;
            }

            if (input == "solve")
            {
                config.maxDepth = Utils::Input::promptInt("Enter search depth (1-3)", 1, 1, 3);

                std::cout << "Calculating best guesses...\n";
                Mastermind::Result result = Mastermind::runMastermindSolverWithEntropy(allPatterns, guessHistory, config);

                printResults(result);
                std::cout << "\nSolver completed.\n\n";
                continue;
            }

            // Try to parse as pattern and feedback
            try
            {
                size_t pipePos = input.find('|');
                if (pipePos == std::string::npos)
                {
                    std::cout << "Error: Missing '|' separator. Use format: 1 2 3 4|2 1\n";
                    continue;
                }

                std::string patternStr = input.substr(0, pipePos);
                std::string feedbackStr = input.substr(pipePos + 1);

                // Parse pattern colors
                std::istringstream patternIss(patternStr);
                std::vector<uint8_t> colors;
                std::string token;
                while (patternIss >> token)
                {
                    int color = std::stoi(token);
                    if (color < 0 || color > static_cast<int>(config.numColors))
                    {
                        throw std::invalid_argument("Color out of range");
                    }
                    colors.push_back(static_cast<uint8_t>(color));
                }

                if (colors.size() != config.numPegs)
                {
                    std::cout << "Error: Pattern must have exactly " << config.numPegs << " colors.\n";
                    continue;
                }

                // Parse feedback
                std::istringstream feedbackIss(feedbackStr);
                int correctPos, correctCol;
                if (!(feedbackIss >> correctPos >> correctCol))
                {
                    std::cout << "Error: Invalid feedback format.\n";
                    continue;
                }

                if (correctPos > static_cast<int>(config.numPegs) || correctCol > static_cast<int>(config.numPegs) ||
                    correctPos < 0 || correctCol < 0 || (correctPos + correctCol) > static_cast<int>(config.numPegs))
                {
                    std::cout << "Error: Invalid feedback values.\n";
                    continue;
                }

                Mastermind::Pattern pattern(colors);
                Mastermind::Feedback feedback;
                feedback.guess = pattern;
                feedback.correctPosition = static_cast<uint8_t>(correctPos);
                feedback.correctColor = static_cast<uint8_t>(correctCol);

                guessHistory.push_back(feedback);
                std::cout << "Added guess: " << pattern.toString() << " with feedback "
                          << correctPos << " " << correctCol << "\n\n";
            }
            catch (const std::exception &e)
            {
                std::cout << "Error: " << e.what() << "\n";
                std::cout << "Please use format: 1 2 3 4|2 1\n\n";
            }
        }
    }

    void MastermindGame::runHeadless(const std::map<std::string, std::string> &args)
    {
        try
        {
            Mastermind::Config config = getConfigFromArgs(args);
            std::vector<Mastermind::Feedback> guessHistory = getFeedbackFromArgs(args, config);

            // Generate all possible patterns
            std::vector<Mastermind::Pattern> allPatterns = Mastermind::generateAllPatterns(config);

            Mastermind::Result result = Mastermind::runMastermindSolverWithEntropy(allPatterns, guessHistory, config);

            std::string possibleFile = Utils::Input::getArgValue(args, "possibleFile", std::string("results/possible.txt"));
            std::string guessesFile = Utils::Input::getArgValue(args, "guessesFile", std::string("results/guesses.txt"));

            saveResults(result, possibleFile, guessesFile);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }

    void MastermindGame::runGUI()
    {
        std::cout << "GUI mode not yet implemented for Mastermind.\n";
    }
}