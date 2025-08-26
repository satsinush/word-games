#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <set>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <iomanip>

#include "utils.hpp"
#include "letterBoxed.hpp"
#include "spellingBee.hpp"
#include "wordle.hpp"
#include "mastermind.hpp"

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

// Helper: Parse Letter Boxed config from command line args
bool parseLetterBoxedArgs(int argc, char *argv[], LetterBoxed::Config &config)
{
    if (argc < 3)
        return false;
    std::string letters = argv[2];
    letters.erase(std::remove_if(letters.begin(), letters.end(), ::isspace), letters.end());
    if (letters.size() != 12)
        return false;
    for (size_t i = 0; i < 12; ++i)
    {
        if (!isalpha(static_cast<unsigned char>(letters[i])))
            return false;
        config.allLetters[i] = std::tolower(static_cast<unsigned char>(letters[i]));
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
    config.charToIndexMap.fill(-1);
    for (int i = 0; i < 12; ++i)
        config.charToIndexMap[static_cast<unsigned char>(config.allLetters[i])] = i;
    // Preset selection by number
    config.maxDepth = 2;
    config.minWordLength = 3;
    config.minUniqueLetters = 2;
    config.pruneRedundantPaths = true;
    config.pruneDominatedClasses = false;
    if (argc > 3)
    {
        int preset = std::stoi(argv[3]);
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
            config.minUniqueLetters = 1;
            config.pruneRedundantPaths = false;
            config.pruneDominatedClasses = false;
        }
        else if (preset == 0 && argc >= 9)
        {
            // Custom
            config.maxDepth = std::stoi(argv[4]);
            config.minWordLength = std::stoi(argv[5]);
            config.minUniqueLetters = std::stoi(argv[6]);
            config.pruneRedundantPaths = std::stoi(argv[7]) != 0;
            config.pruneDominatedClasses = std::stoi(argv[8]) != 0;
        }
    }
    return true;
}

// Helper: Parse Spelling Bee config from command line args
bool parseSpellingBeeArgs(int argc, char *argv[], SpellingBee::Config &config)
{
    if (argc < 3)
        return false;
    std::string letters = argv[2];
    letters.erase(std::remove_if(letters.begin(), letters.end(), ::isspace), letters.end());
    if (letters.size() != 7)
        return false;
    std::set<char> seen;
    for (size_t i = 0; i < 7; ++i)
    {
        char c = std::tolower(static_cast<unsigned char>(letters[i]));
        if (!isalpha(static_cast<unsigned char>(letters[i])))
            return false;
        if (seen.count(c))
            return false;
        seen.insert(c);
        config.allLetters[i] = c;
    }
    for (char c : config.allLetters)
        config.validLettersMap[static_cast<unsigned char>(c)] = true;
    return true;
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

// --- Wordle UI and Game Loop ---
void runWordleGame(const std::vector<WordUtils::Word> &allWordsVec, bool logData = false)
{
    ProfilerUtils::Profiler profiler;
    std::vector<Wordle::Feedback> feedbackHistory;

    while (true)
    {
        std::cout << "\n=== WORDLE SOLVER ===\n";
        std::cout << "Enter your guesses and their feedback patterns.\n";
        std::cout << "Format: WORD 01201 (0=grey, 1=yellow, 2=green)\n";
        std::cout << "Enter 'solve' to get best guesses, 'clear' to start over, 'q' to quit\n\n";

        if (!feedbackHistory.empty())
        {
            std::cout << "Current feedback history:\n";
            for (const auto &fb : feedbackHistory)
            {
                std::cout << "  " << fb.word << " ";
                for (int i = 0; i < 5; ++i)
                {
                    std::cout << fb.getColor(i);
                }
                std::cout << "\n";
            }
            std::cout << "\n";
        }

        while (true)
        {
            std::cout << "Enter guess (or command): ";
            std::string input;
            std::getline(std::cin, input);
            input = WordUtils::trimToLower(input);

            if (input.empty())
                continue;

            if (input == "q")
                return;

            if (input == "clear")
            {
                feedbackHistory.clear();
                std::cout << "Feedback history cleared.\n\n";
                break;
            }

            if (input == "solve")
            {
                std::cout << "Calculating best guesses...\n";

                // Ask for maxDepth configuration
                std::cout << "Enter search depth (1-3, default 1): ";
                std::string depthInput;
                std::getline(std::cin, depthInput);
                int maxDepth = 1;
                if (!depthInput.empty())
                {
                    try
                    {
                        maxDepth = std::stoi(depthInput);
                        if (maxDepth < 1 || maxDepth > 3)
                        {
                            std::cout << "Invalid depth, using default of 1.\n";
                            maxDepth = 1;
                        }
                    }
                    catch (...)
                    {
                        std::cout << "Invalid input, using default of 1.\n";
                        maxDepth = 1;
                    }
                }

                profiler.start();

                Wordle::Config config;
                config.maxDepth = maxDepth;

                Wordle::Result result =
                    Wordle::runWordleSolverWithEntropy(allWordsVec, feedbackHistory, config);

                profiler.end();
                if (logData)
                    profiler.logProfilerData();

                std::cout << "\nPossible remaining words: " << result.totalPossibleWords << "\n";

                if (result.sortedGuesses.empty())
                {
                    std::cout << "No valid words found!\n";
                }
                else
                {
                    std::cout << "\nBest guesses (sorted by information value):\n";

                    // Create header with entropy columns
                    std::cout << "Word\t\t";
                    for (int i = 0; i < config.maxDepth; i++)
                    {
                        std::cout << "E" << (i + 1) << "\t";
                    }
                    std::cout << "Probability\n";

                    std::cout << "----\t\t";
                    for (int i = 0; i < config.maxDepth; i++)
                    {
                        std::cout << "-------\t";
                    }
                    std::cout << "-----------\n";

                    int displayCount = std::min(20, static_cast<int>(result.sortedGuesses.size()));
                    for (int i = 0; i < displayCount; ++i)
                    {
                        const auto &guess = result.sortedGuesses[i];
                        std::cout << guess.word.wordString << ",";

                        // Display probability
                        std::cout << std::fixed << std::setprecision(4) << guess.probability;

                        // Display all entropy levels
                        for (int j = 0; j < config.maxDepth && j < guess.entropyList.size(); j++)
                        {
                            std::cout << "," << std::fixed << std::setprecision(3) << guess.entropyList[j];
                        }

                        std::cout << "\n";
                    }

                    if (result.totalPossibleWords <= 20)
                    {
                        std::cout << "\nAll remaining possibilities:\n";
                        // Show the first few possibilities (which are the actual possible words when maxDepth=0)
                        Wordle::Config possibleConfig;
                        possibleConfig.maxDepth = 0;
                        Wordle::Result possibleResult =
                            Wordle::runWordleSolverWithEntropy(allWordsVec, feedbackHistory, possibleConfig);

                        std::vector<std::string> possibleWords;
                        for (const auto &guess : possibleResult.sortedGuesses)
                        {
                            possibleWords.push_back(guess.word.wordString);
                        }
                        std::sort(possibleWords.begin(), possibleWords.end());
                        for (const auto &word : possibleWords)
                        {
                            std::cout << word << " ";
                        }
                        std::cout << "\n";
                    }
                }

                std::cout << "\nSolver completed in " << profiler.getTotalTime() << " seconds.\n\n";
                continue;
            }

            // Try to parse as feedback
            try
            {
                Wordle::Feedback fb = Wordle::parseFeedback(input);
                feedbackHistory.push_back(fb);
                std::cout << "Added: " << fb.word << " ";
                for (int i = 0; i < 5; ++i)
                {
                    std::cout << fb.getColor(i);
                }
                std::cout << "\n\n";
            }
            catch (const std::exception &e)
            {
                std::cout << "Invalid format. Use: WORD 01201 (5 letters, 5 digits 0-2)\n";
                std::cout << "Error: " << e.what() << "\n\n";
            }
        }
    }
}

void runMastermindGame(bool logData = false)
{
    ProfilerUtils::Profiler profiler;
    std::vector<Mastermind::Feedback> guessHistory;

    // Get configuration
    Mastermind::Config config;
    std::cout << "=== Mastermind Solver ===\n";
    std::cout << "Enter number of pegs (default 4): ";
    std::string input;
    std::getline(std::cin, input);
    if (!input.empty())
    {
        try
        {
            config.numPegs = std::stoi(input);
            if (config.numPegs < 1 || config.numPegs > 20)
            {
                std::cout << "Invalid number of pegs, using default of 4.\n";
                config.numPegs = 4;
            }
        }
        catch (...)
        {
            std::cout << "Invalid input, using default of 4.\n";
        }
    }

    std::cout << "Enter number of colors (default 6): ";
    std::getline(std::cin, input);
    if (!input.empty())
    {
        try
        {
            config.numColors = std::stoi(input);
            if (config.numColors < 1 || config.numColors > 20)
            {
                std::cout << "Invalid number of colors, using default of 6.\n";
                config.numColors = 6;
            }
        }
        catch (...)
        {
            std::cout << "Invalid input, using default of 6.\n";
        }
    }

    std::cout << "Allow duplicate colors? (y/n, default y): ";
    std::getline(std::cin, input);
    if (!input.empty() && (input[0] == 'n' || input[0] == 'N'))
    {
        config.allowDuplicates = false;
    }

    // Generate all possible patterns
    std::vector<Mastermind::Pattern> allPatterns = Mastermind::generateAllPatterns(config);
    std::cout << "Generated " << allPatterns.size() << " possible patterns.\n\n";

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
        std::cout << "  Or enter: 'PATTERN|FEEDBACK' (e.g., '1 2 3 4|2 1' for pattern [1,2,3,4] with 2 correct positions, 1 correct color)\n";
        std::cout << "\nEnter command: ";

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
            std::cout << "Calculating best guesses...\n";

            // Ask for maxDepth configuration
            std::cout << "Enter search depth (1-3, default 1): ";
            std::string depthInput;
            std::getline(std::cin, depthInput);
            int maxDepth = 1;
            if (!depthInput.empty())
            {
                try
                {
                    maxDepth = std::stoi(depthInput);
                    if (maxDepth < 1 || maxDepth > 3)
                    {
                        std::cout << "Invalid depth, using default of 1.\n";
                        maxDepth = 1;
                    }
                }
                catch (...)
                {
                    std::cout << "Invalid input, using default of 1.\n";
                    maxDepth = 1;
                }
            }

            config.maxDepth = maxDepth;
            profiler.start();

            Mastermind::Result result = Mastermind::runMastermindSolverWithEntropy(allPatterns, guessHistory, config);

            profiler.end();

            if (result.sortedGuesses.empty())
            {
                std::cout << "No valid patterns found!\n";
            }
            else
            {
                std::cout << "\nBest guesses (sorted by information value):\n";

                // Create header with entropy columns
                std::cout << "Pattern\t\t\t";
                for (int i = 0; i < config.maxDepth; i++)
                {
                    std::cout << "E" << (i + 1) << "\t";
                }
                std::cout << "Probability\n";

                std::cout << "-------\t\t\t";
                for (int i = 0; i < config.maxDepth; i++)
                {
                    std::cout << "-------\t";
                }
                std::cout << "-----------\n";

                int displayCount = std::min(20, static_cast<int>(result.sortedGuesses.size()));
                for (int i = 0; i < displayCount; ++i)
                {
                    const auto &guess = result.sortedGuesses[i];
                    std::cout << guess.pattern.toString() << ",";

                    // Display probability
                    std::cout << std::fixed << std::setprecision(4) << guess.probability;

                    // Display all entropy levels
                    for (int j = 0; j < config.maxDepth && j < guess.entropyList.size(); j++)
                    {
                        std::cout << "," << std::fixed << std::setprecision(3) << guess.entropyList[j];
                    }

                    std::cout << "\n";
                }

                if (result.totalPossiblePatterns <= 20)
                {
                    std::cout << "\nAll remaining possibilities:\n";
                    // Show just the possible patterns
                    Mastermind::Config possibleConfig = config;
                    possibleConfig.maxDepth = 0;
                    Mastermind::Result possibleResult = Mastermind::runMastermindSolverWithEntropy(allPatterns, guessHistory, possibleConfig);

                    std::vector<std::string> possiblePatterns;
                    for (const auto &guess : possibleResult.sortedGuesses)
                    {
                        possiblePatterns.push_back(guess.pattern.toString());
                    }
                    std::sort(possiblePatterns.begin(), possiblePatterns.end());
                    for (const auto &pattern : possiblePatterns)
                    {
                        std::cout << pattern << " ";
                    }
                    std::cout << "\n";
                }
            }

            std::cout << "\nSolver completed in " << profiler.getTotalTime() << " seconds.\n\n";
            continue;
        }

        // Try to parse as pattern and feedback
        try
        {
            // Split input by pipe separator
            size_t pipePos = input.find('|');
            if (pipePos == std::string::npos)
            {
                throw std::runtime_error("Missing pipe separator '|' between pattern and feedback");
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
                if (color < 0 || color >= config.numColors)
                {
                    throw std::runtime_error("Color " + std::to_string(color) + " out of range (0-" + std::to_string(config.numColors - 1) + ")");
                }
                colors.push_back(static_cast<uint8_t>(color));
            }

            if (colors.size() != config.numPegs)
            {
                throw std::runtime_error("Expected " + std::to_string(config.numPegs) + " colors, got " + std::to_string(colors.size()));
            }

            // Parse feedback
            std::istringstream feedbackIss(feedbackStr);
            int correctPos, correctCol;
            if (!(feedbackIss >> correctPos >> correctCol))
            {
                throw std::runtime_error("Expected 2 feedback numbers (correct position, correct color)");
            }

            if (correctPos < 0 || correctPos > config.numPegs || correctCol < 0 || correctCol > config.numPegs)
            {
                throw std::runtime_error("Feedback values out of range");
            }

            Mastermind::Pattern pattern(colors);
            Mastermind::Feedback feedback;
            feedback.guess = pattern;
            feedback.correctPosition = static_cast<uint8_t>(correctPos);
            feedback.correctColor = static_cast<uint8_t>(correctCol);

            guessHistory.push_back(feedback);
            std::cout << "Added: " << pattern.toString() << " -> "
                      << static_cast<int>(feedback.correctPosition) << " "
                      << static_cast<int>(feedback.correctColor) << "\n\n";
        }
        catch (const std::exception &e)
        {
            std::cout << "Invalid format. Use: 'pattern|feedback' where:\n";
            std::cout << "  pattern: " << config.numPegs << " colors separated by spaces\n";
            std::cout << "  feedback: 2 numbers (correct position, correct color)\n";
            std::cout << "Example for " << config.numPegs << " pegs: '";
            for (int i = 0; i < config.numPegs; i++)
            {
                if (i > 0)
                    std::cout << " ";
                std::cout << (i % config.numColors);
            }
            std::cout << "|2 1' (pattern with 2 correct positions, 1 correct color)\n";
            std::cout << "Error: " << e.what() << "\n\n";
        }
    }
}

// --- Helper: Parse flags from argv ---
struct CmdArgs
{
    std::string mode;
    std::string letters;
    int preset = -1;
    int maxDepth = -1;
    int minWordLength = -1;
    int minUniqueLetters = -1;
    int pruneRedundantPaths = -1;
    int pruneDominatedClasses = -1;
    int excludeUncommonWords = -1;
    int start = 0;                                     // for read mode
    int end = -1;                                      // for read mode
    std::string file = "results/temp.txt";             // default file for output/input (legacy)
    std::string possibleFile = "results/possible.txt"; // file for possible words
    std::string guessesFile = "results/guesses.txt";   // file for guesses with entropy
    // Mastermind-specific
    int numPegs = 4;             // number of pegs in mastermind
    int numColors = 6;           // number of colors in mastermind
    bool allowDuplicates = true; // allow duplicate colors in mastermind
    bool valid = false;
};

CmdArgs parseFlags(int argc, char *argv[])
{
    CmdArgs args;
    int customFlagCount = 0;
    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--mode" && i + 1 < argc)
        {
            args.mode = argv[++i];
        }
        else if (a == "--letters" && i + 1 < argc)
        {
            args.letters = argv[++i];
        }
        else if (a == "--preset" && i + 1 < argc)
        {
            args.preset = std::stoi(argv[++i]);
        }
        else if (a == "--maxDepth" && i + 1 < argc)
        {
            args.maxDepth = std::stoi(argv[++i]);
        }
        else if (a == "--minWordLength" && i + 1 < argc)
        {
            args.minWordLength = std::stoi(argv[++i]);
        }
        else if (a == "--minUniqueLetters" && i + 1 < argc)
        {
            args.minUniqueLetters = std::stoi(argv[++i]);
        }
        else if (a == "--pruneRedundantPaths" && i + 1 < argc)
        {
            args.pruneRedundantPaths = std::stoi(argv[++i]);
        }
        else if (a == "--pruneDominatedClasses" && i + 1 < argc)
        {
            args.pruneDominatedClasses = std::stoi(argv[++i]);
        }
        else if (a == "--excludeUncommonWords" && i + 1 < argc)
        {
            args.excludeUncommonWords = std::stoi(argv[++i]);
        }
        else if (a == "--start" && i + 1 < argc)
        {
            args.start = std::stoi(argv[++i]);
        }
        else if (a == "--end" && i + 1 < argc)
        {
            args.end = std::stoi(argv[++i]);
        }
        else if (a == "--file" && i + 1 < argc)
        {
            args.file = argv[++i];
        }
        else if (a == "--possibleFile" && i + 1 < argc)
        {
            args.possibleFile = argv[++i];
        }
        else if (a == "--guessesFile" && i + 1 < argc)
        {
            args.guessesFile = argv[++i];
        }
        else if (a == "--numPegs" && i + 1 < argc)
        {
            args.numPegs = std::stoi(argv[++i]);
        }
        else if (a == "--numColors" && i + 1 < argc)
        {
            args.numColors = std::stoi(argv[++i]);
        }
        else if (a == "--allowDuplicates" && i + 1 < argc)
        {
            args.allowDuplicates = (std::stoi(argv[++i]) != 0);
        }
    }
    args.valid = true;
    if (args.mode.empty() && args.letters.empty())
    {
        std::cout << "Mode and letters are required arguments.\n";
        args.valid = false;
    }
    if (args.mode == "letterboxed" && (args.preset < 1 || args.preset > 3) && (args.maxDepth == -1 || args.minWordLength == -1 || args.minUniqueLetters == -1 || args.pruneRedundantPaths == -1 || args.pruneDominatedClasses == -1))
    {
        std::cout << "Invalid argument combination.\n";
        args.valid = false;
    }
    if (args.mode == "read" && (args.start < 0 || args.end < args.start))
    {
        std::cout << "Invalid read range.\n";
        args.valid = false;
    }
    return args;
}

// --- Combined Main Loop ---
int main(int argc, char *argv[])
{
    std::vector<WordUtils::Word> allWordsVec = WordUtils::loadWords();
    bool logData = false;

    CmdArgs cmd = parseFlags(argc, argv);

    // Print usage if argument is "help"
    if (argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "help" || std::string(argv[1]) == "-h" || !cmd.valid))
    {
        std::cout << "Usage:\n";
        std::cout << "  --mode <mode>: Specify the mode of operation. Options are:\n";
        std::cout << "      letterboxed: Solve the Letter Boxed puzzle.\n";
        std::cout << "      spellingbee: Solve the Spelling Bee puzzle.\n";
        std::cout << "      wordle: Solve Wordle puzzles with entropy-based suggestions.\n";
        std::cout << "      mastermind: Solve Mastermind puzzles with entropy-based suggestions.\n";
        std::cout << "      read: Read and display results from a file.\n";
        std::cout << "\n";

        std::cout << "  Letter Boxed:\n";
        std::cout << "    " << argv[0] << " --mode letterboxed --letters <12letters> [--preset <1|2|3|0>] [--file <filename>]\n";
        std::cout << "      --letters: Specify the 12 letters for the Letter Boxed puzzle.\n";
        std::cout << "      --preset: 1=Default, 2=Fast, 3=Thorough, 0=Custom. (optional)\n";
        std::cout << "      --maxDepth: Maximum number of words per solution (required if preset=0).\n";
        std::cout << "      --minWordLength: Minimum word length (required if preset=0).\n";
        std::cout << "      --minUniqueLetters: Minimum unique letters per word (required if preset=0).\n";
        std::cout << "      --pruneRedundantPaths: 0 or 1 to enable/disable pruning redundant paths (required if preset=0).\n";
        std::cout << "      --pruneDominatedClasses: 0 or 1 to enable/disable pruning dominated classes (required if preset=0).\n";
        std::cout << "      --file: Specify the output file to save solutions (default: temp.txt).\n";
        std::cout << "\n";

        std::cout << "  Spelling Bee:\n";
        std::cout << "    " << argv[0] << " --mode spellingbee --letters <7letters> [--file <filename>]\n";
        std::cout << "      --letters: Specify the 7 letters for the Spelling Bee puzzle.\n";
        std::cout << "      --file: Specify the output file to save solutions (default: temp.txt).\n";
        std::cout << "\n";

        std::cout << "  Wordle:\n";
        std::cout << "    " << argv[0] << " --mode wordle --guesses \"STEAL 01201\" \"CRANE 00120\" [--maxDepth <depth>] [--possibleFile <filename>] [--guessesFile <filename>] [--excludeUncommonWords <0|1>]\n";
        std::cout << "      --guesses: Specify guess/feedback pairs. Format: \"WORD 01201\" where:\n";
        std::cout << "                 0=grey (letter not in word), 1=yellow (letter in word, wrong position),\n";
        std::cout << "                 2=green (letter in word, correct position)\n";
        std::cout << "      --maxDepth: Search depth for entropy calculation (0-2, default: 0). Higher values are more accurate but slower.\n";
        std::cout << "      --possibleFile: Output file for possible solution words (default: results/possible.txt).\n";
        std::cout << "      --guessesFile: Output file for all guesses with entropy/probability (default: results/guesses.txt).\n";
        std::cout << "      --excludeUncommonWords: 0 or 1 to enable/disable excluding uncommon words (default: 0).\n";
        std::cout << "\n";

        std::cout << "  Mastermind:\n";
        std::cout << "    " << argv[0] << " --mode mastermind --guesses \"1 2 3 4|2 2\" [--numPegs <pegs>] [--numColors <colors>] [--allowDuplicates <0|1>] [--maxDepth <depth>] [--possibleFile <filename>] [--guessesFile <filename>]\n";
        std::cout << "      --guesses: Specify guess/feedback pairs. Format: \"1 2 3 4|2 2\" where:\n";
        std::cout << "                 Pattern: sequence of color numbers separated by spaces\n";
        std::cout << "                 Feedback: <correct_position> <correct_color> (e.g., \"2 2\" = 2 correct position, 2 correct color)\n";
        std::cout << "      --numPegs: Number of pegs in the pattern (default: 4)\n";
        std::cout << "      --numColors: Number of available colors (default: 6)\n";
        std::cout << "      --allowDuplicates: 0 or 1 to disable/enable duplicate colors in patterns (default: 1)\n";
        std::cout << "      --maxDepth: Search depth for entropy calculation (1-3, default: 1)\n";
        std::cout << "      --possibleFile: Output file for possible solution patterns (default: results/possible.txt)\n";
        std::cout << "      --guessesFile: Output file for all guesses with entropy/probability (default: results/guesses.txt)\n";
        std::cout << "\n";

        std::cout << "  Read Mode:\n";
        std::cout << "    " << argv[0] << " --mode read [--file <filename>] [--start <startIndex>] [--end <endIndex>]\n";
        std::cout << "      --file: Specify the input file to read solutions from (default: temp.txt).\n";
        std::cout << "      --start: Starting index of results to display (default: 0).\n";
        std::cout << "      --end: Ending index of results to display (default: all results).\n";
        std::cout << "\n";

        std::cout << "  Help:\n";
        std::cout << "    " << argv[0] << " --help\n";
        std::cout << "      Displays this help message with detailed information about arguments and options.\n";
        return 0;
    }

    if (cmd.valid)
    {
        // Ensure the directory for the specified file exists
        std::filesystem::path filePath(cmd.file);
        if (!filePath.parent_path().empty() && !std::filesystem::exists(filePath.parent_path()))
        {
            try
            {
                std::filesystem::create_directories(filePath.parent_path());
            }
            catch (const std::filesystem::filesystem_error &e)
            {
                std::cerr << "Error: Could not create directory for file: " << e.what() << "\n";
                return 1;
            }
        }

        if (cmd.mode == "letterboxed")
        {
            LetterBoxed::Config config;
            std::string letters = cmd.letters;
            letters.erase(std::remove_if(letters.begin(), letters.end(), ::isspace), letters.end());
            if (letters.size() != 12)
            {
                std::cout << "Invalid Letter Boxed letters.\n";
                return 1;
            }
            for (size_t i = 0; i < 12; ++i)
            {
                if (!isalpha(static_cast<unsigned char>(letters[i])))
                {
                    std::cout << "Invalid Letter Boxed letters.\n";
                    return 1;
                }
                config.allLetters[i] = std::tolower(static_cast<unsigned char>(letters[i]));
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
            config.charToIndexMap.fill(-1);
            for (int i = 0; i < 12; ++i)
                config.charToIndexMap[static_cast<unsigned char>(config.allLetters[i])] = i;

            // If no preset is supplied, assume custom
            bool presetSupplied = false;
            for (int i = 1; i < argc; ++i)
            {
                if (std::string(argv[i]) == "--preset")
                {
                    presetSupplied = true;
                    break;
                }
            }

            // --- Preset logic with override ---
            if (presetSupplied)
            {
                // Set defaults for the preset
                if (cmd.preset == 1)
                {
                    config.maxDepth = 2;
                    config.minWordLength = 3;
                    config.minUniqueLetters = 2;
                    config.pruneRedundantPaths = true;
                    config.pruneDominatedClasses = false;
                }
                else if (cmd.preset == 2)
                {
                    config.maxDepth = 2;
                    config.minWordLength = 4;
                    config.minUniqueLetters = 3;
                    config.pruneRedundantPaths = true;
                    config.pruneDominatedClasses = true;
                }
                else if (cmd.preset == 3)
                {
                    config.maxDepth = 3;
                    config.minWordLength = 3;
                    config.minUniqueLetters = 1;
                    config.pruneRedundantPaths = false;
                    config.pruneDominatedClasses = false;
                }
                // Override with any supplied custom arguments
                if (cmd.maxDepth != -1)
                    config.maxDepth = cmd.maxDepth;
                if (cmd.minWordLength != -1)
                    config.minWordLength = cmd.minWordLength;
                if (cmd.minUniqueLetters != -1)
                    config.minUniqueLetters = cmd.minUniqueLetters;
                if (cmd.pruneRedundantPaths != -1)
                    config.pruneRedundantPaths = cmd.pruneRedundantPaths != 0;
                if (cmd.pruneDominatedClasses != -1)
                    config.pruneDominatedClasses = cmd.pruneDominatedClasses != 0;
            }
            else
            {
                // Require all custom arguments
                if (cmd.maxDepth == -1 || cmd.minWordLength == -1 || cmd.minUniqueLetters == -1 || cmd.pruneRedundantPaths == -1 || cmd.pruneDominatedClasses == -1)
                {
                    std::cout << "Missing custom arguments. Required: --maxDepth --minWordLength --minUniqueLetters --pruneRedundantPaths --pruneDominatedClasses\n";
                    return 1;
                }
                config.maxDepth = cmd.maxDepth;
                config.minWordLength = cmd.minWordLength;
                config.minUniqueLetters = cmd.minUniqueLetters;
                config.pruneRedundantPaths = cmd.pruneRedundantPaths != 0;
                config.pruneDominatedClasses = cmd.pruneDominatedClasses != 0;
            }

            int totalLetterCount = 0;
            for (const auto &word : allWordsVec)
                totalLetterCount += word.wordString.size();
            std::vector<LetterBoxed::Solution> finalSolutions = LetterBoxed::runLetterBoxedSolver(config, allWordsVec, totalLetterCount);
            std::ofstream tempFile(cmd.file);
            for (const auto &sol : finalSolutions)
            {
                tempFile << sol.text << "\n";
            }
            tempFile.close();
            std::cout << finalSolutions.size() << "\n";
            std::cout << cmd.file;
            return 0;
        }
        else if (cmd.mode == "spellingbee")
        {
            SpellingBee::Config config;
            std::string letters = cmd.letters;
            letters.erase(std::remove_if(letters.begin(), letters.end(), ::isspace), letters.end());
            if (letters.size() != 7)
            {
                std::cout << "Invalid Spelling Bee letters.\n";
                return 1;
            }
            std::set<char> seen;
            for (size_t i = 0; i < 7; ++i)
            {
                char c = std::tolower(static_cast<unsigned char>(letters[i]));
                if (!isalpha(static_cast<unsigned char>(letters[i])) || seen.count(c))
                {
                    std::cout << "Invalid Spelling Bee letters.\n";
                    return 1;
                }
                seen.insert(c);
                config.allLetters[i] = c;
            }
            for (char c : config.allLetters)
                config.validLettersMap[static_cast<unsigned char>(c)] = true;
            std::vector<WordUtils::Word> solutions = SpellingBee::runSpellingBeeSolver(allWordsVec, config);
            std::ofstream tempFile(cmd.file);
            for (const auto &w : solutions)
            {
                tempFile << w.wordString << "\n";
            }
            tempFile.close();
            std::cout << solutions.size() << "\n";
            std::cout << cmd.file;
            return 0;
        }
        else if (cmd.mode == "wordle")
        {
            // Example usage:
            // --mode wordle --guesses "STEAL 01201" "CRANE 00120" ...
            std::vector<Wordle::Feedback> feedbacks;
            for (int i = 1; i < argc; ++i)
            {
                if (std::string(argv[i]) == "--guesses")
                {
                    for (int j = i + 1; j < argc && argv[j][0] != '-'; ++j)
                    {
                        feedbacks.push_back(Wordle::parseFeedback(argv[j]));
                    }
                }
            }

            ProfilerUtils::Profiler profiler;
            // Get possible words and best guesses with entropy
            Wordle::Config config;
            config.maxDepth = (cmd.maxDepth != -1) ? cmd.maxDepth : 1; // Use command line depth or default to 1
            config.excludeUncommonWords = (cmd.excludeUncommonWords == 1) ? true : false;

            profiler.start();
            Wordle::Result result =
                Wordle::runWordleSolverWithEntropy(allWordsVec, feedbacks, config);
            profiler.end();
            profiler.logProfilerData();

            // Use the specific file arguments
            std::string possibleWordsFile = cmd.possibleFile;
            std::string guessesFile = cmd.guessesFile;

            // Write possible words to first file (sorted alphabetically)
            std::ofstream possibleFile(possibleWordsFile);
            std::vector<std::string> possibleWords;
            for (const auto &guess : result.sortedGuesses)
            {
                if (guess.probability > 0)
                {
                    possibleWords.push_back(guess.word.wordString);
                }
            }
            std::sort(possibleWords.begin(), possibleWords.end());
            for (const auto &word : possibleWords)
            {
                possibleFile << word << "\n";
            }
            possibleFile.close();

            // Write all guesses with entropy and probability to second file
            std::ofstream guessFile(guessesFile);
            for (const auto &guess : result.sortedGuesses)
            {
                guessFile << guess.word.wordString << ",";
                guessFile << std::fixed << std::setprecision(4) << guess.probability;

                // Write all entropy levels
                for (int j = 0; j < config.maxDepth && j < guess.entropyList.size(); j++)
                {
                    guessFile << "," << std::fixed << std::setprecision(3) << guess.entropyList[j];
                }

                guessFile << "\n";
            }
            guessFile.close();

            // Display summary to console
            std::cout << result.totalPossibleWords << "\n";
            std::cout << result.sortedGuesses.size() << "\n";
            std::cout << possibleWordsFile << "\n";
            std::cout << guessesFile << "\n";
            return 0;
        }
        else if (cmd.mode == "mastermind")
        {
            // Parse mastermind feedback guesses
            std::vector<Mastermind::Feedback> feedbacks;
            for (int i = 1; i < argc; ++i)
            {
                if (std::string(argv[i]) == "--guesses")
                {
                    for (int j = i + 1; j < argc && argv[j][0] != '-'; ++j)
                    {
                        feedbacks.push_back(Mastermind::parseFeedback(argv[j], cmd.numPegs));
                    }
                }
            }

            ProfilerUtils::Profiler profiler;
            // Get possible patterns and best guesses with entropy
            Mastermind::Config config;
            config.numPegs = cmd.numPegs;
            config.numColors = cmd.numColors;
            config.allowDuplicates = cmd.allowDuplicates;
            config.maxDepth = (cmd.maxDepth != -1) ? cmd.maxDepth : 0;

            // Generate all possible patterns
            std::vector<Mastermind::Pattern> allPatterns = Mastermind::generateAllPatterns(config);

            profiler.start();
            Mastermind::Result result =
                Mastermind::runMastermindSolverWithEntropy(allPatterns, feedbacks, config);
            profiler.end();
            profiler.logProfilerData();

            // Use the specific file arguments
            std::string possiblePatternsFile = cmd.possibleFile;
            std::string guessesFile = cmd.guessesFile;

            // Write possible patterns to first file (sorted alphabetically)
            std::ofstream possibleFile(possiblePatternsFile);
            std::vector<std::string> possiblePatterns;
            for (const auto &guess : result.sortedGuesses)
            {
                if (guess.probability > 0)
                {
                    std::string patternStr;
                    for (uint8_t color : guess.pattern.colors)
                    {
                        if (!patternStr.empty())
                            patternStr += " ";
                        patternStr += std::to_string((int)color);
                    }
                    possiblePatterns.push_back(patternStr);
                }
            }
            std::sort(possiblePatterns.begin(), possiblePatterns.end());
            for (const auto &pattern : possiblePatterns)
            {
                possibleFile << pattern << "\n";
            }
            possibleFile.close();

            // Write all guesses with entropy and probability to second file
            std::ofstream guessFile(guessesFile);
            for (const auto &guess : result.sortedGuesses)
            {
                for (uint8_t color : guess.pattern.colors)
                {
                    guessFile << (int)color << " ";
                }
                guessFile << ",";
                guessFile << std::fixed << std::setprecision(4) << guess.probability;

                // Write all entropy levels
                for (int j = 0; j < config.maxDepth && j < guess.entropyList.size(); j++)
                {
                    guessFile << "," << std::fixed << std::setprecision(3) << guess.entropyList[j];
                }

                guessFile << "\n";
            }
            guessFile.close();

            // Display summary to console
            std::cout << result.totalPossiblePatterns << "\n";
            std::cout << result.sortedGuesses.size() << "\n";
            std::cout << possiblePatternsFile << "\n";
            std::cout << guessesFile << "\n";
            return 0;
        }
        else if (cmd.mode == "read")
        {
            // Read and page through the specified file
            std::ifstream tempFile(cmd.file);
            if (!tempFile.is_open())
            {
                std::cout << "Could not open " << cmd.file << "\n";
                return 1;
            }
            std::vector<std::string> lines;
            std::string line;
            while (std::getline(tempFile, line))
            {
                lines.push_back(line);
            }
            tempFile.close();
            int start = std::max(0, cmd.start);
            int end = (cmd.end == -1) ? static_cast<int>(lines.size()) : std::min(cmd.end, static_cast<int>(lines.size()));
            if (start >= end || start >= static_cast<int>(lines.size()))
            {
                std::cout << "No results in specified range.\n";
                return 0;
            }
            for (int i = start; i < end; ++i)
            {
                std::cout << lines[i] << "\n";
            }
            return 0;
        }
        else
        {
            std::cout << "Unknown mode. Use --mode letterboxed, --mode spellingbee, --mode wordle, or --mode read.\n";
            return 1;
        }
    }

    while (true)
    {
        std::cout << "\nSelect game mode:\n";
        std::cout << "  1: Letter Boxed\n";
        std::cout << "  2: Spelling Bee\n";
        std::cout << "  3: Wordle\n";
        std::cout << "  4: Mastermind\n";
        std::cout << "  q: Quit\n";
        std::cout << "Enter choice: ";
        std::string input;
        std::getline(std::cin, input);
        input = WordUtils::trimToLower(input);
        if (input.empty())
            continue;
        if (input == "q")
            break;
        if (input == "1")
            runLetterBoxedGame(allWordsVec, logData);
        else if (input == "2")
            runSpellingBeeGame(allWordsVec, logData);
        else if (input == "3")
            runWordleGame(allWordsVec, logData);
        else if (input == "4")
            runMastermindGame(logData);
        else
            std::cout << "Invalid choice. Try again.\n";
    }
    return 0;
}
