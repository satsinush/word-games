#define NOMINMAX

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
#include "utils/utils.hpp"

#include "dungleon/DungleonGame.hpp"
#include "wordle/WordleGame.hpp"

#ifdef WITH_GUI
#include <QApplication>
#include <QIcon>

#include "gui/MainWindow.hpp"
#endif

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>
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
  dungleon               Solve Dungleon puzzles with entropy-based suggestions
  read                   Read and display results from a file

Letter Boxed:
  %s letterboxed --letters <12letters> [OPTIONS]
    --letters <letters>         12 letters for the puzzle (required)
    --preset <1|2|3|0>         Preset configuration (default: 1)
                                 1=Default, 2=Fast, 3=Thorough, 0=Custom
                                 Note: Individual options override preset values
    --max-depth <n>            Maximum words per solution (can override preset)
    --min-word-length <n>      Minimum word length (can override preset)
    --min-unique-letters <n>   Minimum unique letters per word (can override preset)
    --prune-paths              Enable pruning redundant paths (can override preset)
    --prune-classes            Enable pruning dominated classes (can override preset)
    -o, --output <file>        Output file (default: results/temp.txt)

Spelling Bee:
  %s spellingbee --letters <letters> [OPTIONS]
    --letters <letters>         Letters for the puzzle, minimum 3, duplicates allowed (required)
    --exclude-uncommon-words    Exclude uncommon words (1/true/yes or 0/false/no, default: 0)
    --must-include-first-letter Must include first letter (1/true/yes or 0/false/no, default: 1)
    --reuse-letters            Allow reuse of letters (1/true/yes or 0/false/no, default: 1)
    -o, --output <file>        Output file (default: results/temp.txt)

Wordle:
  %s wordle [OPTIONS]
    --guesses <guesses>        Guess/feedback pairs. Format: "STEAL 01201;CRANE 00120"
                                 0=grey, 1=yellow, 2=green
    --word-length <n>          Word length (default: 5, range: 1-32)
    --max-depth <0-2>          Search depth for entropy (default: 0)
    --exclude-uncommon-words   Exclude uncommon words (1/true/yes or 0/false/no, default: 0)
    -o, --output <file>        Output file with possible words and all guesses
                                 (default: results/guesses.txt)

Mastermind:
  %s mastermind [OPTIONS]
    --guesses <guesses>        Guess/feedback pairs. Format: "RGBC 1 2;MYRC 1 2"
                                 Pattern then black pegs and white pegs
    --pegs <n>                 Number of pegs (default: 4, range: 1-20)
    --colors <chars>           Available color characters (default: "RGBCMY")
    --allow-duplicates         Allow duplicate colors (1/true/yes or 0/false/no, default: 1)
    --max-depth <0-2>          Search depth for entropy (default: 1, range: 0-2)
    -o, --output <file>        Output file with possible patterns and all guesses
                                 (default: results/guesses.txt)

Dungleon:
  %s dungleon [OPTIONS]
    --guesses <guesses>        Guess/feedback pairs. Format: "ar kn ma bt dr 01234"
                                 Character pairs with colors (0-4)
    --solutions <solutions>    Past solutions for Gauntlet mode. Format: "ar kn ma bt dr"
    --max-depth <0-2>          Search depth for entropy (default: 0, range: 0-2)
    --exclude-impossible       Exclude impossible patterns from guesses (1/true/yes or 0/false/no, default: 0)
    -o, --output <file>        Output file with possible patterns and all guesses
                                 (default: results/dungleon.txt)

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
  %s spellingbee --letters abcdefg --must-include-first-letter 1 --reuse-letters 1
  %s wordle --guesses "STEAL 01201;CRANE 00120" --word-length 5 --max-depth 1
  %s mastermind --guesses "RGBC 1 2" --pegs 4 --colors "RGBCMY" --max-depth 1
  %s dungleon --guesses "ar kn bo ne fr 00010" --max-depth 1
  %s -i
  %s read results/wordle.txt --start 0 --end 10
)";

  printf(usage_message, programName, programName, programName, programName,
         programName, programName, programName, programName, programName,
         programName, programName, programName, programName, programName);
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

    // Load the application icon using the configured resource path
    std::string iconPath = Utils::getResourceFile("icon.ico");
    QApplication::setWindowIcon(QIcon(QString::fromStdString(iconPath)));

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
#if defined(WITH_GUI) && defined(_WIN32)
  // For Windows GUI applications, attach to parent console if available
  bool needsConsole = (argc > 1); // Has command line arguments
  bool attachedToConsole = false;

  if (needsConsole) {
    // Try to attach to parent console (if launched from cmd/powershell)
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
      // Successfully attached to parent console
      freopen_s((FILE **)stdout, "CONOUT$", "w", stdout);
      freopen_s((FILE **)stderr, "CONOUT$", "w", stderr);
      freopen_s((FILE **)stdin, "CONIN$", "r", stdin);
      attachedToConsole = true;

      // Print a newline to separate from the command that launched us
      std::cout << std::endl;
    }
    // If AttachConsole fails, we're likely launched from GUI (file explorer,
    // etc.) In that case, we just won't have console output, which is fine for
    // GUI apps
  }
#endif

#ifdef TRACY_ENABLE
  ZoneScoped;
#if defined(WITH_GUI) && defined(_WIN32)
  if (attachedToConsole)
#endif
    std::cout << "Tracy Profiler enabled." << std::endl;
#endif

  int result = run(argc, argv);

#ifdef TRACY_ENABLE
  FrameMark;
#endif

#if defined(WITH_GUI) && defined(_WIN32)
  // Clean up console if we attached to it
  if (attachedToConsole) {
    // Print a newline before returning control to parent console
    std::cout << std::endl;
    FreeConsole();
  }
#endif

  return result;
}