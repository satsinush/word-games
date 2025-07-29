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

// --- UI Drawing and Input Functions ---

/**
 * @brief Draws the puzzle box with the current letters.
 * @param letters The array of letters to display.
 * @param clearScreen Whether to clear the console before drawing.
 */
void drawPuzzleBox(const std::array<char, 12> &letters, bool clearScreen = true)
{
    if (clearScreen)
    {
#ifdef _WIN32
        system("cls");
#else
        std::cout << "\033[2J\033[1;1H";
#endif
    }

    auto up = [](char c)
    { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); };

    std::cout << "--- NYT Letter Boxed Solver ---" << std::endl
              << std::endl;
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
 * @brief Prompts the user for a value using std::cin, with a default fallback.
 * @param prompt The message to display to the user.
 * @param defaultValue The value to use if the user enters nothing.
 * @return The user's input or the default value.
 */
std::string promptForValue(const std::string &prompt, const std::string &defaultValue)
{
    std::cout << prompt << " (default: " << defaultValue << "): ";
    std::string input;
    std::getline(std::cin, input);
    return input.empty() ? defaultValue : input;
}

/**
 * @brief Gathers all puzzle settings from the user via a sequential console input process.
 * @return A Config struct populated with the user's settings.
 */
Config getUserConfiguration()
{
    Config config;
    config.letters.fill('*');

    // --- Step 1: Get and confirm the 12 puzzle letters ---
    int letterIndex = 0;
    while (true)
    {
        drawPuzzleBox(config.letters);

        // Display the appropriate prompt based on the current state
        if (letterIndex < 12)
        {
            std::cout << "Enter letter " << letterIndex + 1 << " of 12. Use Backspace to correct." << std::endl;
        }
        else
        {
            std::cout << "All letters entered. Press Enter to confirm, or Backspace to change." << std::endl;
        }

        char key = _getch();

        // Handle logic based on whether we are confirming or still entering letters
        if (letterIndex >= 12)
        {
            if (key == '\r')
            { // Enter confirms and breaks the loop
                break;
            }
            else if (key == 8)
            { // Backspace allows editing
                config.letters[letterIndex] = '*';
                config.letters[--letterIndex] = '_';
            }
        }
        else
        { // Still entering letters
            if (isalpha(static_cast<unsigned char>(key)))
            {
                config.letters[letterIndex++] = tolower(static_cast<unsigned char>(key));
            }
            else if (key == 8 && letterIndex > 0)
            { // Backspace
                config.letters[letterIndex] = '*';
                config.letters[--letterIndex] = '_';
            }
        }
    }

    // --- Step 2: Get solver options using std::cin ---
    drawPuzzleBox(config.letters);
    std::cout << "Configure solver options. Press Enter to accept the default value." << std::endl
              << std::endl;

    std::string minLengthStr = promptForValue("Enter minimum word length", "3");
    std::string minUniqueStr = promptForValue("Enter minimum unique letters per word", "2");
    std::string maxDepthStr = promptForValue("Enter max solution depth (words)", "3");
    std::string prunePathsStr = promptForValue("Prune redundant paths? (1=Y, 0=N)", "1");
    std::string pruneClassesStr = promptForValue("Prune dominated classes? (1=Y, 0=N)", "1");

    // Parse final values from strings into the config struct
    try
    {
        config.minWordLength = std::stoi(minLengthStr);
        config.minUniqueLetters = std::stoi(minUniqueStr);
        config.maxDepth = std::stoi(maxDepthStr);
        config.pruneRedundantPaths = (prunePathsStr != "0");
        config.pruneDominatedClasses = (pruneClassesStr != "0");
    }
    catch (const std::invalid_argument &e)
    {
        std::cout << "Invalid number entered. Using default values for all options." << std::endl;
        // Let the default-constructed values in Config persist.
    }

    std::cout << std::endl;
    return config;
}

/**
 * @brief Main entry point for the application.
 */
int main()
{
    Utils::Profiler profiler;

    while (true)
    {
        Config config = getUserConfiguration();

        // Load dictionary
        std::cout << "Reading word file...\n\n";
        std::filesystem::path exe_path = std::filesystem::current_path();
        std::ifstream file(exe_path / "data" / "words.txt");

        std::vector<std::string> allDictionaryWords;
        std::string line;
        if (file.is_open())
        {
            while (std::getline(file, line))
            {
                // Sanitize dictionary words to lowercase
                std::transform(line.begin(), line.end(), line.begin(),
                               [](unsigned char c)
                               { return std::tolower(c); });
                allDictionaryWords.push_back(line);
            }
            file.close();
            std::cout << "Loaded " << allDictionaryWords.size() << " words from dictionary.\n"
                      << std::endl;
        }
        else
        {
            std::cerr << "Error: Could not open words.txt. Please ensure it's in a 'data' sub-directory.\n";
            std::cout << "\nPress any key to exit.\n";
            _getch();
            return 1;
        }

        // Setup PuzzleData from the user's configuration
        PuzzleData puzzleData;
        puzzleData.allLetters = config.letters;
        for (int i = 0; i < 3; ++i)
            puzzleData.letterToSideMapping[i] = 0; // Top
        for (int i = 3; i < 6; ++i)
            puzzleData.letterToSideMapping[i] = 1; // Right
        for (int i = 6; i < 9; ++i)
            puzzleData.letterToSideMapping[i] = 2; // Bottom
        for (int i = 9; i < 12; ++i)
            puzzleData.letterToSideMapping[i] = 3; // Left
        for (int i = 0; i < 12; ++i)
            puzzleData.uniquePuzzleLetters.set(i);

        std::vector<Solution> finalSolutions;

        // Run the solver
        profiler.start();
        runLetterBoxedSolver(
            puzzleData,
            allDictionaryWords,
            config,
            finalSolutions);
        profiler.end();

        profiler.logProfilerData();

        // Print results
        for (const auto &s : finalSolutions)
        {
            std::cout << s.text << "\n";
        }
        if (!finalSolutions.empty())
        {
            std::cout << "\n";
        }

        std::cout << finalSolutions.size() << " total solution(s) found.\n";
        std::cout << "\nPress 'q' to quit, or any other key to run again.\n";
        char ch = _getch();
        if (ch == 'q' || ch == 'Q')
            break;
    }
    return 0;
}
