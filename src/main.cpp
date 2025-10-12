#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <fstream>
#include <algorithm>

#include "utils/utils.hpp"
#include "game/Game.hpp"
#include "letterBoxed/LetterBoxedGame.hpp"
#include "spellingBee/SpellingBeeGame.hpp"
#include "wordle/WordleGame.hpp"
#include "mastermind/MastermindGame.hpp"

#ifdef WITH_GUI
#include <QApplication>
#include "gui/MainWindow.hpp"
#endif

void printUsage(const char *programName)
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
    std::cout << "    " << programName << " --mode letterboxed --letters <12letters> [--preset <1|2|3|0>] [--file <filename>]\n";
    std::cout << "      --letters: Specify the 12 letters for the Letter Boxed puzzle.\n";
    std::cout << "      --preset: 1=Default, 2=Fast, 3=Thorough, 0=Custom. (optional)\n";
    std::cout << "      --maxDepth: Maximum number of words per solution (required if preset=0).\n";
    std::cout << "      --minWordLength: Minimum word length (required if preset=0).\n";
    std::cout << "      --minUniqueLetters: Minimum unique letters per word (required if preset=0).\n";
    std::cout << "      --pruneRedundantPaths: 0 or 1 to enable/disable pruning redundant paths (required if preset=0).\n";
    std::cout << "      --pruneDominatedClasses: 0 or 1 to enable/disable pruning dominated classes (required if preset=0).\n";
    std::cout << "      --file: Specify the output file to save solutions (default: results/temp.txt).\n";
    std::cout << "\n";

    std::cout << "  Spelling Bee:\n";
    std::cout << "    " << programName << " --mode spellingbee --letters <7letters> [--file <filename>]\n";
    std::cout << "      --letters: Specify the 7 letters for the Spelling Bee puzzle.\n";
    std::cout << "      --file: Specify the output file to save solutions (default: results/temp.txt).\n";
    std::cout << "\n";

    std::cout << "  Wordle:\n";
    std::cout << "    " << programName << " --mode wordle [--guesses <guesses>] [--maxDepth <depth>] [--possibleFile <filename>] [--guessesFile <filename>] [--excludeUncommonWords <0|1>]\n";
    std::cout << "      --guesses: Specify guess/feedback pairs. Format: \"STEAL 01201;CRANE 00120\" where:\n";
    std::cout << "                 0=grey (letter not in word), 1=yellow (letter in word, wrong position),\n";
    std::cout << "                 2=green (letter in word, correct position)\n";
    std::cout << "      --maxDepth: Search depth for entropy calculation (0-2, default: 0). Higher values are more accurate but slower.\n";
    std::cout << "      --possibleFile: Output file for possible solution words (default: results/possible.txt).\n";
    std::cout << "      --guessesFile: Output file for all guesses with entropy/probability (default: results/guesses.txt).\n";
    std::cout << "      --excludeUncommonWords: 0 or 1 to enable/disable excluding uncommon words (default: 0).\n";
    std::cout << "\n";

    std::cout << "  Mastermind:\n";
    std::cout << "    " << programName << " --mode mastermind [--guesses <guesses>] [--numPegs <pegs>] [--numColors <colors>] [--allowDuplicates <0|1>] [--maxDepth <depth>] [--possibleFile <filename>] [--guessesFile <filename>]\n";
    std::cout << "      --guesses: Specify guess/feedback pairs. Format: \"1 1 2 2 3|1 2;3 4 5 6 7|1 2\" where:\n";
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
    std::cout << "    " << programName << " --mode read [--file <filename>] [--start <startIndex>] [--end <endIndex>]\n";
    std::cout << "      --file: Specify the input file to read solutions from (default: results/temp.txt).\n";
    std::cout << "      --start: Starting index of results to display (default: 0).\n";
    std::cout << "      --end: Ending index of results to display (default: all results).\n";
    std::cout << "\n";

    std::cout << "  Help:\n";
    std::cout << "    " << programName << " --help\n";
    std::cout << "      Displays this help message with detailed information about arguments and options.\n";
}

void runReadMode(const std::map<std::string, std::string> &args)
{
    std::string filename = Utils::Input::getArgValue(args, "file", std::string("results/temp.txt"));
    int start = Utils::Input::getArgValue(args, "start", 0);
    int end = Utils::Input::getArgValue(args, "end", -1);

    // Read and page through the specified file
    std::ifstream tempFile(filename);
    if (!tempFile.is_open())
    {
        std::cerr << "Could not open " << filename << "\n";
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(tempFile, line))
    {
        lines.push_back(line);
    }
    tempFile.close();

    int actualStart = std::max(0, start);
    int actualEnd = (end == -1) ? static_cast<int>(lines.size()) : std::min(end, static_cast<int>(lines.size()));

    if (actualStart >= actualEnd || actualStart >= static_cast<int>(lines.size()))
    {
        std::cerr << "No results in specified range.\n";
        return;
    }

    for (int i = actualStart; i < actualEnd; ++i)
    {
        std::cout << lines[i] << "\n";
    }
}

std::unique_ptr<Game::IGame> createGame(const std::string &mode, const std::vector<Utils::Word> &wordVec)
{
    if (mode == "letterboxed")
        return std::make_unique<Game::LetterBoxedGame>(wordVec);
    else if (mode == "spellingbee")
        return std::make_unique<Game::SpellingBeeGame>(wordVec);
    else if (mode == "wordle")
        return std::make_unique<Game::WordleGame>(wordVec);
    else if (mode == "mastermind")
        return std::make_unique<Game::MastermindGame>();
    else
        return nullptr;
}

void runInteractiveMode(const std::vector<Utils::Word> &wordVec)
{
    while (true)
    {
        std::cout << "\nSelect game mode:\n";
        std::cout << "  1: Letter Boxed\n";
        std::cout << "  2: Spelling Bee\n";
        std::cout << "  3: Wordle\n";
        std::cout << "  4: Mastermind\n";
        std::cout << "  5: Read Results File\n";
        std::cout << "  q: Quit\n";
        std::cout << "Enter choice: ";

        std::string input;
        std::getline(std::cin, input);
        input = Utils::trimToLower(input);

        if (input.empty())
            continue;
        if (input == "q")
            return;

        if (input == "5")
        {
            // Interactive read mode
            std::string filename = Utils::Input::promptString("Enter filename to read", "results/temp.txt");
            int start = Utils::Input::promptInt("Enter starting line number", 0, 0);
            int end = Utils::Input::promptInt("Enter ending line number (-1 for all)", -1, -1);

            std::map<std::string, std::string> readArgs;
            readArgs["file"] = filename;
            readArgs["start"] = std::to_string(start);
            if (end != -1)
                readArgs["end"] = std::to_string(end);

            try
            {
                runReadMode(readArgs);
            }
            catch (const std::exception &e)
            {
                std::cout << "Error: " << e.what() << "\n";
            }
            continue;
        }

        std::unique_ptr<Game::IGame> game;
        if (input == "1")
            game = createGame("letterboxed", wordVec);
        else if (input == "2")
            game = createGame("spellingbee", wordVec);
        else if (input == "3")
            game = createGame("wordle", wordVec);
        else if (input == "4")
            game = createGame("mastermind", wordVec);
        else
        {
            std::cout << "Invalid choice. Please try again.\n";
            continue;
        }

        if (game)
        {
            game->runCLI();
        }
    }
}

int main(int argc, char *argv[])
{
    // Load words (not needed for Mastermind, but we'll load them anyway for consistency)
    std::vector<Utils::Word> wordVec = Utils::loadWords();

#ifdef WITH_GUI
    // Check if no arguments are provided - launch GUI
    if (argc == 1)
    {
        QApplication app(argc, argv);

        MainWindow window;
        window.show();

        return app.exec();
    }
#endif

    // Parse command line arguments
    std::map<std::string, std::string> args = Utils::Input::parseCommandArgs(argc, argv);

    // Check for help
    if (argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "help" || std::string(argv[1]) == "-h"))
    {
        printUsage(argv[0]);
        return 0;
    }

    // Check if mode is specified for headless operation
    if (args.find("mode") != args.end())
    {
        std::string mode = Utils::Input::getArgValue(args, "mode", std::string(""));

        // Handle read mode separately since it doesn't use the game interface
        if (mode == "read")
        {
            try
            {
                runReadMode(args);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error: " << e.what() << "\n";
                return 1;
            }
            return 0;
        }

        auto game = createGame(mode, wordVec);

        if (!game)
        {
            std::cerr << "Invalid mode: " << mode << "\n";
            printUsage(argv[0]);
            return 1;
        }

        try
        {
            Utils::g_profiler.start();
            game->runHeadless(args);
            Utils::g_profiler.stop();
            Utils::g_profiler.logProfilerData();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }

        return 0;
    }

    // Run interactive mode if no mode specified
    runInteractiveMode(wordVec);

    return 0;
}