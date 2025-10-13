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
  --mode <mode>: Specify the mode of operation. Options are:
      letterboxed: Solve the Letter Boxed puzzle.
      spellingbee: Solve the Spelling Bee puzzle.
      wordle: Solve Wordle puzzles with entropy-based suggestions.
      mastermind: Solve Mastermind puzzles with entropy-based suggestions.
      read: Read and display results from a file.

  Letter Boxed:
    %s --mode letterboxed --letters <12letters> [--preset <1|2|3|0>] [--file <filename>]
      --letters: Specify the 12 letters for the Letter Boxed puzzle.
      --preset: 1=Default, 2=Fast, 3=Thorough, 0=Custom. (optional)
      --maxDepth: Maximum number of words per solution (required if preset=0).
      --minWordLength: Minimum word length (required if preset=0).
      --minUniqueLetters: Minimum unique letters per word (required if preset=0).
      --pruneRedundantPaths: 0 or 1 to enable/disable pruning redundant paths (required if preset=0).
      --pruneDominatedClasses: 0 or 1 to enable/disable pruning dominated classes (required if preset=0).
      --file: Specify the output file to save solutions (default: results/temp.txt).

  Spelling Bee:
    %s --mode spellingbee --letters <7letters> [--file <filename>]
      --letters: Specify the 7 letters for the Spelling Bee puzzle.
      --file: Specify the output file to save solutions (default: results/temp.txt).

  Wordle:
    %s --mode wordle [--guesses <guesses>] [--maxDepth <depth>] [--possibleFile <filename>] [--guessesFile <filename>] [--excludeUncommonWords <0|1>]
      --guesses: Specify guess/feedback pairs. Format: "STEAL 01201;CRANE 00120" where:
                  0=grey (letter not in word), 1=yellow (letter in word, wrong position),
                  2=green (letter in word, correct position)
      --maxDepth: Search depth for entropy calculation (0-2, default: 0). Higher values are more accurate but slower.
      --possibleFile: Output file for possible solution words (default: results/possible.txt).
      --guessesFile: Output file for all guesses with entropy/probability (default: results/guesses.txt).
      --excludeUncommonWords: 0 or 1 to enable/disable excluding uncommon words (default: 0).

  Mastermind:
    %s --mode mastermind [--guesses <guesses>] [--numPegs <pegs>] [--numColors <colors>] [--allowDuplicates <0|1>] [--maxDepth <depth>] [--possibleFile <filename>] [--guessesFile <filename>]
      --guesses: Specify guess/feedback pairs. Format: "1 1 2 2 3|1 2;3 4 5 6 7|1 2" where:
                  Pattern: sequence of color numbers separated by spaces
                  Feedback: <correct_position> <correct_color> (e.g., "2 2" = 2 correct position, 2 correct color)
      --numPegs: Number of pegs in the pattern (default: 4)
      --numColors: Number of available colors (default: 6)
      --allowDuplicates: 0 or 1 to disable/enable duplicate colors in patterns (default: 1)
      --maxDepth: Search depth for entropy calculation (1-3, default: 1)
      --possibleFile: Output file for possible solution patterns (default: results/possible.txt)
      --guessesFile: Output file for all guesses with entropy/probability (default: results/guesses.txt)

  Read Mode:
    %s --mode read [--file <filename>] [--start <startIndex>] [--end <endIndex>]
      --file: Specify the input file to read solutions from (default: results/temp.txt).
      --start: Starting index of results to display (default: 0).
      --end: Ending index of results to display (default: all results).

  Benchmarking:
    %s --benchmark runtime --mode <mode> [--iterations <count>] [--verbose <0|1>]
      Runs a runtime benchmark for the specified game mode.
      --iterations: Number of iterations to run (default: 1)
      --verbose: Enable verbose output during benchmark (default: 0)

    %s --benchmark performance --mode <mode> [--verbose <0|1>]
      Runs a performance benchmark for the specified game mode.
      Currently only available for Wordle - tests solver against top 1000 words.
      --verbose: Enable verbose output during benchmark (default: 0)

  Help:
    %s --help
      Displays this help message with detailed information about arguments and options.
)";

  printf(usage_message, programName, programName, programName, programName,
         programName, programName, programName, programName);
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
#ifdef WITH_GUI
  // Check if no arguments are provided - launch GUI
  if (argc == 1) {
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
  }
#endif

  // Parse command line arguments
  std::map<std::string, std::string> args =
      Utils::Input::parseCommandArgs(argc, argv);

  // Check for help
  if (argc > 1 &&
      (std::string(argv[1]) == "--help" || std::string(argv[1]) == "help" ||
       std::string(argv[1]) == "-h")) {
    printUsage(argv[0]);
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
      Utils::g_profiler.start();
      game->runHeadless(args);
      Utils::g_profiler.stop();
      Utils::g_profiler.logProfilerData();
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