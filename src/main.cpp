#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "game/Game.hpp"
#include "letterBoxed/LetterBoxedGame.hpp"
#include "mastermind/MastermindGame.hpp"
#include "spellingBee/SpellingBeeGame.hpp"
#include "utils/testingUtils.hpp"
#include "utils/utils.hpp"
#include "wordle/WordleGame.hpp"

#ifdef WITH_GUI
#include <QApplication>

#include "gui/MainWindow.hpp"
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
    --file <filename>          Output file (default: results/temp.txt)

Spelling Bee:
  %s spellingbee --letters <7letters> [OPTIONS]
    --letters <letters>         7 letters for the puzzle (required)
    --file <filename>          Output file (default: results/temp.txt)

Wordle:
  %s wordle [OPTIONS]
    --guesses <guesses>        Guess/feedback pairs. Format: "STEAL 01201;CRANE 00120"
                                 0=grey, 1=yellow, 2=green
    --max-depth <0-2>          Search depth for entropy (default: 0)
    --possible-file <file>     Output file for possible words (default: results/possible.txt)
    --guesses-file <file>      Output file for all guesses (default: results/guesses.txt)
    --exclude-uncommon-words   Exclude uncommon words (1/true/yes or 0/false/no)

Mastermind:
  %s mastermind [OPTIONS]
    --guesses <guesses>        Guess/feedback pairs. Format: "1 1 2 2|1 2;3 4 5 6|1 2"
                                 Pattern|Feedback (pos color)
    --num-pegs <n>             Number of pegs (default: 4)
    --num-colors <n>           Number of colors (default: 6)
    --allow-duplicates         Allow duplicate colors (1/true/yes or 0/false/no, default: 1)
    --max-depth <1-3>          Search depth for entropy (default: 1)
    --possible-file <file>     Output file for possible patterns (default: results/possible.txt)
    --guesses-file <file>      Output file for all guesses (default: results/guesses.txt)

Read Mode:
  %s read [OPTIONS]
    --file <filename>          Input file to read (default: results/temp.txt)
    --start <n>                Starting index (default: 0)
    --end <n>                  Ending index (default: all)

Benchmarking:
  %s --benchmark runtime <mode> [OPTIONS]
    Run runtime benchmark for specified game mode
    --iterations <n>           Number of iterations (default: 1)
    --verbose                  Enable verbose output

  %s --benchmark performance <mode> [OPTIONS]
    Run performance benchmark (currently Wordle only)
    --verbose                  Enable verbose output

Boolean Values:
  Accepted as TRUE:  1, y, Y, yes, YES, Yes, t, T, true, True, TRUE
  Accepted as FALSE: 0, n, N, no, NO, No, f, F, false, False, FALSE

Examples:
  %s letterboxed --letters abcdefghijkl --preset 2
  %s -i
  %s wordle --guesses "STEAL 01201;CRANE 00120" --max-depth 1
)";

  printf(usage_message, programName, programName, programName, programName,
         programName, programName, programName, programName, programName,
         programName, programName);
}

void runReadMode(const std::map<std::string, std::string> &args) {
  std::string filename =
      Utils::Input::getArgValue(args, "file", std::string("results/temp.txt"));
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

std::unique_ptr<Game::IGame>
createGame(const std::string &mode, const std::vector<Utils::Word> &wordVec) {
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

void runInteractiveMode(const std::vector<Utils::Word> &wordVec) {
  while (true) {
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

    if (input == "5") {
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
      game = createGame("letterboxed", wordVec);
    else if (input == "2")
      game = createGame("spellingbee", wordVec);
    else if (input == "3")
      game = createGame("wordle", wordVec);
    else if (input == "4")
      game = createGame("mastermind", wordVec);
    else {
      std::cout << "Invalid choice. Please try again.\n";
      continue;
    }

    if (game) {
      game->runCLI();
    }
  }
}

int main(int argc, char *argv[]) {
  // Parse command line arguments
  std::map<std::string, std::string> args =
      Utils::Input::parseCommandArgs(argc, argv);

  // Check for help
  if (args.find("h") != args.end() || args.find("help") != args.end()) {
    printUsage(argv[0]);
    return 0;
  }

#ifdef WITH_GUI
  // Check if no arguments are provided or no mode specified - launch GUI
  if (argc == 1) {
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
  }
#endif

  // Check if interactive mode is requested
  if (args.find("i") != args.end()) {
    std::vector<Utils::Word> wordVec = Utils::loadWords();
    runInteractiveMode(wordVec);
    return 0;
  }

  // Load words (not needed for Mastermind, but we'll load them anyway for
  // consistency)
  std::vector<Utils::Word> wordVec = Utils::loadWords();

  // Check for benchmark mode
  if (args.find("benchmark") != args.end()) {
    std::string benchmarkType =
        Utils::Input::getArgValue(args, "benchmark", std::string(""));

    if (benchmarkType != "runtime" && benchmarkType != "performance") {
      std::cerr << "Invalid benchmark type: " << benchmarkType << "\n";
      std::cerr << "Valid options: runtime, performance\n";
      return 1;
    }

    if (args.find("mode") == args.end()) {
      std::cerr << "Mode must be specified for benchmarking\n";
      return 1;
    }

    std::string mode = Utils::Input::getArgValue(args, "mode", std::string(""));
    Utils::Testing::BenchmarkConfig config =
        Utils::Testing::parseBenchmarkArgs(args);

    try {
      Utils::Testing::BenchmarkResult result;

      if (benchmarkType == "runtime") {
        result = Utils::Testing::runRuntimeBenchmark(mode, wordVec, config);
      } else if (benchmarkType == "performance") {
        result = Utils::Testing::runPerformanceBenchmark(mode, wordVec, config);
      }

      Utils::Testing::printBenchmarkResults(result);
    } catch (const std::exception &e) {
      std::cerr << "Benchmark error: " << e.what() << "\n";
      return 1;
    }

    return 0;
  }

  // Check if mode is specified for headless operation
  if (args.find("mode") != args.end()) {
    std::string mode = Utils::Input::getArgValue(args, "mode", std::string(""));

    // Handle read mode separately since it doesn't use the game interface
    if (mode == "read") {
      try {
        runReadMode(args);
      } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
      }
      return 0;
    }

    auto game = createGame(mode, wordVec);

    if (!game) {
      std::cerr << "Invalid mode: " << mode << "\n";
      printUsage(argv[0]);
      return 1;
    }

    try {
      Utils::Profiling::g_profiler.start();
      game->runHeadless(args);
      Utils::Profiling::g_profiler.stop();
      Utils::Profiling::g_profiler.logProfilerData();
    } catch (const std::exception &e) {
      std::cerr << "Error: " << e.what() << "\n";
      return 1;
    }

    return 0;
  }

  // Run interactive mode if no mode specified
  runInteractiveMode(wordVec);

  return 0;
}