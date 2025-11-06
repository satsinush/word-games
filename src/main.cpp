#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "dungleon/dungleon.hpp"
#include "game/Game.hpp"
#include "letterBoxed/LetterBoxedGame.hpp"
#include "mastermind/MastermindGame.hpp"
#include "spellingBee/SpellingBeeGame.hpp"
#include "utils/inputUtils.hpp"
#include "utils/wordUtils.hpp"

#include "dungleon/DungleonGame.hpp"
#include "wordle/WordleGame.hpp"

#ifdef WITH_GUI
#include <QApplication>
#include <QIcon>

#include "gui/MainWindow.hpp"
#endif

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

void printUsage(const char *programName) {
  const char *usage_message = R"(Usage:
  %s [OPTIONS] [MODE]
  
  If no arguments provided: Launch GUI mode (if compiled with GUI support)
  If MODE is provided: Run the specified game mode
  
Options:
  -h, --help              Display this help message
  -v, --verbose           Enable verbose output (for future use)
  -i                      Run in interactive CLI mode

Modes:
  letterboxed            Solve the Letter Boxed puzzle
  spellingbee            Solve the Spelling Bee puzzle
  wordle                 Solve Wordle puzzles with entropy-based suggestions
  mastermind             Solve Mastermind puzzles with entropy-based suggestions
  read                   Read and display results from a file

Letter Boxed:
  %s letterboxed --letters <12letters> [OPTIONS]
    --letters <letters>         12 letters for the puzzle (required)
    --preset <1|2|3|0>         Preset configuration (default: 1)
                                 1=Default, 2=Fast, 3=Thorough, 0=Custom
    --max-depth <n>            Maximum words per solution (required if preset=0)
    --min-word-length <n>      Minimum word length (required if preset=0)
    --min-unique-letters <n>   Minimum unique letters per word (required if preset=0)
    --prune-paths              Enable pruning redundant paths (1/true/yes or 0/false/no)
    --prune-classes            Enable pruning dominated classes (1/true/yes or 0/false/no)
    -o, --output <file>        Output file (default: results/temp.txt)

Spelling Bee:
  %s spellingbee --letters <7letters> [OPTIONS]
    --letters <letters>         7 letters for the puzzle (required)
    -o, --output <file>        Output file (default: results/temp.txt)

Wordle:
  %s wordle [OPTIONS]
    --guesses <guesses>        Guess/feedback pairs. Format: "STEAL 01201;CRANE 00120"
                                 0=grey, 1=yellow, 2=green
    --max-depth <0-2>          Search depth for entropy (default: 0)
    -o, --output <file>        Output file with possible words and all guesses
                                 (default: results/guesses.txt)
    --exclude-uncommon-words   Exclude uncommon words (1/true/yes or 0/false/no)

Mastermind:
  %s mastermind [OPTIONS]
    --guesses <guesses>        Guess/feedback pairs. Format: "1 1 2 2|1 2;3 4 5 6|1 2"
                                 Pattern|Feedback (pos color)
    --num-pegs <n>             Number of pegs (default: 4)
    --num-colors <n>           Number of colors (default: 6)
    --allow-duplicates         Allow duplicate colors (1/true/yes or 0/false/no, default: 1)
    --max-depth <1-3>          Search depth for entropy (default: 1)
    -o, --output <file>        Output file with possible patterns and all guesses
                                 (default: results/guesses.txt)

Read Mode:
  %s read [FILE] [OPTIONS]
    FILE                       Input file to read (default: results/temp.txt)
    --start <n>                Starting index (default: 0)
    --end <n>                  Ending index (default: all)

Boolean Values:
  Accepted as TRUE:  1, y, Y, yes, YES, Yes, t, T, true, True, TRUE
  Accepted as FALSE: 0, n, N, no, NO, No, f, F, false, False, FALSE

Examples:
  %s letterboxed --letters abcdefghijkl --preset 2 -o results/solutions.txt
  %s -i
  %s wordle --guesses "STEAL 01201;CRANE 00120" --max-depth 1 -o results/wordle.txt
  %s read results/wordle.txt --start 0 --end 10
)";

  printf(usage_message, programName, programName, programName, programName,
         programName, programName, programName, programName, programName,
         programName);
}

void runReadMode(const std::map<std::string, std::string> &args,
                 const std::vector<std::string> &positional = {}) {
  // Use second positional argument as filename (from command line),
  // or "file" key (from interactive mode), or default
  std::string filename;
  if (positional.size() >= 2) {
    // Command line: second positional arg is the filename
    filename = positional[1];
  } else {
    // Interactive mode: check "file" key
    filename = Utils::Input::getArgValue(args, "file",
                                         std::string("results/temp.txt"));
  }
  int start = Utils::Input::getArgValue(args, "start", 0);
  int end = Utils::Input::getArgValue(args, "end", -1);

  // Read and page through the specified file
  std::ifstream tempFile(filename);
  if (!tempFile.is_open()) {
    std::cerr << "Could not open " << filename << "\n";
    throw std::runtime_error("Could not open file: " + filename);
  }

  std::vector<std::string> lines;
  std::string line;
  while (std::getline(tempFile, line)) {
    lines.push_back(line);
  }
  tempFile.close();

  int actualStart = std::max(0, start);
  int actualEnd = (end == -1) ? static_cast<int>(lines.size())
                              : std::min(end, static_cast<int>(lines.size()));

  if (actualStart >= actualEnd ||
      actualStart >= static_cast<int>(lines.size())) {
    std::cerr << "No results in specified range.\n";
    return;
  }

  for (int i = actualStart; i < actualEnd; ++i) {
    std::cout << lines[i] << "\n";
  }
}

std::unique_ptr<Game::IGame> createGame(const std::string &mode) {
  if (mode == "letterboxed")
    return std::make_unique<Game::LetterBoxedGame>();
  else if (mode == "spellingbee")
    return std::make_unique<Game::SpellingBeeGame>();
  else if (mode == "wordle")
    return std::make_unique<Game::WordleGame>();
  else if (mode == "mastermind")
    return std::make_unique<Game::MastermindGame>();
  else if (mode == "dungleon")
    return std::make_unique<Game::DungleonGame>();
  else
    return nullptr;
}

void runInteractiveMode() {
  while (true) {
    std::cout << "\nSelect game mode:\n";
    std::cout << "  1: Letter Boxed\n";
    std::cout << "  2: Spelling Bee\n";
    std::cout << "  3: Wordle\n";
    std::cout << "  4: Mastermind\n";
    std::cout << "  5: Dungleon\n";
    std::cout << "  6: Read Results File\n";
    std::cout << "  q: Quit\n";
    std::cout << "Enter choice: ";

    std::string input;
    std::getline(std::cin, input);

    // Check for EOF
    if (std::cin.eof()) {
      return;
    }

    input = Utils::trimToLower(input);

    if (input.empty())
      continue;
    if (input == "q")
      return;

    if (input == "6") {
      // Interactive read mode
      std::string filename = Utils::Input::promptString(
          "Enter filename to read", "results/temp.txt");
      int start = Utils::Input::promptInt("Enter starting line number", 0, 0);
      int end = Utils::Input::promptInt("Enter ending line number (-1 for all)",
                                        -1, -1);

      std::map<std::string, std::string> readArgs;
      readArgs["file"] = filename;
      readArgs["start"] = std::to_string(start);
      if (end != -1)
        readArgs["end"] = std::to_string(end);

      try {
        runReadMode(readArgs);
      } catch (const std::exception &e) {
        std::cout << "Error: " << e.what() << "\n";
      }
      continue;
    }

    std::unique_ptr<Game::IGame> game;
    if (input == "1")
      game = createGame("letterboxed");
    else if (input == "2")
      game = createGame("spellingbee");
    else if (input == "3")
      game = createGame("wordle");
    else if (input == "4")
      game = createGame("mastermind");
    else if (input == "5")
      game = createGame("dungleon");
    else {
      std::cout << "Invalid choice. Please try again.\n";
      continue;
    }

    if (game) {
      try {
        game->runCLI();
      } catch (const Utils::Input::UserCancelledException &) {
        std::cout << "Operation cancelled. Returning to main menu.\n";
      }
    }
  }
}

int run(int argc, char *argv[]) {
  // Parse command line arguments
  Utils::Input::CommandArgs cmdArgs =
      Utils::Input::parseCommandArgs(argc, argv);
  std::map<std::string, std::string> &args = cmdArgs.flags;

  // Check for help
  if (args.find("h") != args.end() || args.find("help") != args.end()) {
    printUsage(argv[0]);
    return 0;
  }

  // Load words once to populate global cache
  Utils::loadWords();

#ifdef WITH_GUI
  // Check if no arguments are provided
  if (argc == 1) {
    QApplication app(argc, argv);

    // Set application name shown by the windowing system and used by Qt
    QApplication::setApplicationName("Puzzle++");

    // Load the SVG icon from the repository resources folder and set it as
    // the application / window icon. Use a relative path; this assumes the
    // working directory contains the project resources at runtime (typical
    // when running from the project root during development). If you use a
    // Qt resource (.qrc) in the future, switch to the ":/" prefix.
    QApplication::setWindowIcon(QIcon(QStringLiteral("resources/icon.svg")));

    MainWindow window;
    window.show();

    return app.exec();
  }
#endif

  // Check if interactive mode is requested
  if (args.find("i") != args.end()) {
    runInteractiveMode();
    return 0;
  }

  // Check if mode is specified for headless operation
  if (!cmdArgs.positional.empty()) {
    std::string mode = cmdArgs.positional[0];

    // Handle read mode separately since it doesn't use the game interface
    if (mode == "read") {
      try {
        runReadMode(args, cmdArgs.positional);
      } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
      }
      return 0;
    }

    auto game = createGame(mode);

    if (!game) {
      std::cerr << "Invalid mode: " << mode << "\n";
      printUsage(argv[0]);
      return 1;
    }

    try {
      game->runHeadless(cmdArgs);
    } catch (const std::exception &e) {
      std::cerr << "Error: " << e.what() << "\n";
      return 1;
    }

    return 0;
  }

  // Run interactive mode if no mode specified
  runInteractiveMode();

  return 0;
}

int main(int argc, char *argv[]) {
#ifdef TRACY_ENABLE
  ZoneScoped;
  std::cout << "Tracy Profiler enabled." << std::endl;
#endif

  int result = run(argc, argv);

#ifdef TRACY_ENABLE
  FrameMark;
#endif

  return result;
}