#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "../utils/inputUtils.hpp"
#include "MastermindGame.hpp"

namespace Game {
Mastermind::Config MastermindGame::getConfigFromUser() {
  Mastermind::Config config;

  std::cout << "=== Mastermind Solver ===\n";
  config.numPegs = Utils::Input::promptInt("Enter number of pegs", 4, 1, 20);
  config.numColors =
      Utils::Input::promptInt("Enter number of colors", 6, 1, 20);
  config.allowDuplicates =
      Utils::Input::promptBool("Allow duplicate colors?", true);

  return config;
}

Mastermind::Config MastermindGame::getConfigFromArgs(
    const std::map<std::string, std::string> &args) {
  Mastermind::Config config;
  config.numPegs = Utils::Input::getArgValue(args, "num-pegs", 4u);
  config.numColors = Utils::Input::getArgValue(args, "num-colors", 6u);
  config.allowDuplicates =
      Utils::Input::getArgValue(args, "allow-duplicates", true);
  config.maxDepth = Utils::Input::getArgValue(args, "max-depth", 1u);
  return config;
}

std::vector<Mastermind::Feedback>
MastermindGame::getFeedbackFromUser(const Mastermind::Config &config) {
  std::vector<Mastermind::Feedback> guessHistory;

  std::cout << "\nEnter your guesses and feedback.\n";
  std::cout << "Format: 1 2 3 4|2 1 (pattern|correct_position correct_color)\n";
  std::cout << "Enter 'done' when finished entering feedback.\n\n";

  while (true) {
    std::cout << "Enter guess and feedback (or 'done'): ";
    std::string input;
    std::getline(std::cin, input);

    if (Utils::trimToLower(input) == "done")
      break;
    if (input.empty())
      continue;

    try {
      // Split input by pipe separator
      size_t pipePos = input.find('|');
      if (pipePos == std::string::npos) {
        std::cout << "Error: Missing '|' separator. Use format: 1 2 3 4|2 1\n";
        continue;
      }

      std::string patternStr = input.substr(0, pipePos);
      std::string feedbackStr = input.substr(pipePos + 1);

      // Parse pattern colors directly into Pattern
      std::istringstream patternIss(patternStr);
      Mastermind::Pattern pattern;
      pattern.numPegs = 0;
      std::string token;
      while (patternIss >> token && pattern.numPegs < Mastermind::MAX_PEGS) {
        int color = std::stoi(token);
        if (color < 0 || color > static_cast<int>(config.numColors)) {
          throw std::invalid_argument("Color out of range");
        }
        pattern.colors[pattern.numPegs] = static_cast<uint8_t>(color);
        pattern.numPegs++;
      }

      if (pattern.numPegs != config.numPegs) {
        std::cout << "Error: Pattern must have exactly " << config.numPegs
                  << " colors.\n";
        continue;
      }

      // Parse feedback
      std::istringstream feedbackIss(feedbackStr);
      int correctPos, correctCol;
      if (!(feedbackIss >> correctPos >> correctCol)) {
        std::cout << "Error: Invalid feedback format. Use: correct_position "
                     "correct_color\n";
        continue;
      }

      if (correctPos > static_cast<int>(config.numPegs) ||
          correctCol > static_cast<int>(config.numPegs) || correctPos < 0 ||
          correctCol < 0 ||
          (correctPos + correctCol) > static_cast<int>(config.numPegs)) {
        std::cout << "Error: Invalid feedback values.\n";
        continue;
      }
      Mastermind::Feedback feedback;
      feedback.guess = pattern;
      feedback.correctPosition = static_cast<uint8_t>(correctPos);
      feedback.correctColor = static_cast<uint8_t>(correctCol);

      guessHistory.push_back(feedback);
      std::cout << "Added: " << pattern.toString() << " with feedback "
                << static_cast<int>(feedback.correctPosition) << " "
                << static_cast<int>(feedback.correctColor) << "\n";
    } catch (const std::exception &e) {
      std::cout << "Error parsing input: " << e.what() << "\n";
      std::cout << "Please use format: 1 2 3 4|2 1\n";
    }
  }

  return guessHistory;
}

std::vector<Mastermind::Feedback> MastermindGame::getFeedbackFromArgs(
    const std::map<std::string, std::string> &args,
    const Mastermind::Config &config) {
  std::vector<Mastermind::Feedback> guessHistory;

  // Look for guesses argument
  auto it = args.find("guesses");
  if (it == args.end()) {
    return guessHistory; // Return empty if no guesses provided
  }

  // Parse multiple feedback strings in format: "1 1 2 2|1 2;0 0 0 4|1 2"
  std::istringstream iss(it->second);
  std::string guessStr;

  // Parse each guess separated by semicolons
  while (std::getline(iss, guessStr, ';')) {
    std::string trimmed = Utils::trimToLower(guessStr);
    if (trimmed.empty() || trimmed.find('|') == std::string::npos)
      continue;

    try {
      // Split by pipe
      size_t pipePos = trimmed.find('|');
      std::string patternStr = trimmed.substr(0, pipePos);
      std::string feedbackStr = trimmed.substr(pipePos + 1);

      // Parse pattern directly into Pattern
      std::istringstream patternIss(patternStr);
      Mastermind::Pattern pattern;
      pattern.numPegs = 0;
      std::string token;
      while (patternIss >> token && pattern.numPegs < Mastermind::MAX_PEGS) {
        int color = std::stoi(token);
        if (color < 0 || color > static_cast<int>(config.numColors)) {
          throw std::invalid_argument("Color out of range");
        }
        pattern.colors[pattern.numPegs] = static_cast<uint8_t>(color);
        pattern.numPegs++;
      }

      if (pattern.numPegs != config.numPegs) {
        throw std::invalid_argument("Invalid number of pegs");
      }

      // Parse feedback
      std::istringstream feedbackIss(feedbackStr);
      int correctPos, correctCol;
      if (feedbackIss >> correctPos >> correctCol) {
        Mastermind::Feedback feedback;
        feedback.guess = pattern;
        feedback.correctPosition = static_cast<uint8_t>(correctPos);
        feedback.correctColor = static_cast<uint8_t>(correctCol);
        guessHistory.push_back(feedback);
      }
    } catch (const std::exception &e) {
      std::cerr << "Warning: Could not parse feedback '" << guessStr
                << "': " << e.what() << "\n";
    }
  }

  return guessHistory;
}

void MastermindGame::printResults(const Mastermind::Result &result) {
  std::cout << "\n=== SOLVER RESULTS ===\n";

  if (result.totalPossiblePatterns == 0) {
    std::cout << "No possible patterns found with given constraints.\n";
    return;
  }

  std::cout << "Possible patterns remaining: " << result.totalPossiblePatterns
            << "\n\n";

  if (!result.sortedGuesses.empty()) {
    std::cout << "=== Best guesses ===\n";

    // Header
    std::cout << std::setw(10) << "Rank";
    std::cout << std::setw(25) << "Pattern";
    std::cout << std::setw(12) << "ENT Score";
    std::cout << std::setw(15) << "Probability" << "\n";

    int totalWidth = 10 + 25 + 12 + 15;
    std::cout << std::string(std::max(0, totalWidth), '-') << "\n";

    int possibleCount = 0;
    int i = 0;

    // Print the top 10 guesses first
    while (i < 10 && i < static_cast<int>(result.sortedGuesses.size())) {
      const auto &guess = result.sortedGuesses[i];
      if (guess.probability > 0.0)
        possibleCount++;
      i++;

      std::cout << std::setw(10) << (i);
      std::cout << std::setw(25) << guess.pattern.toString();
      std::cout << std::setw(12) << std::fixed << std::setprecision(3)
                << guess.ent;
      std::cout << std::setw(15) << std::fixed << std::setprecision(6)
                << guess.probability << "\n";
    }

    std::cout << std::string(std::max(0, totalWidth), '-') << "\n";

    // Print the next possible guesses until 10 possible patterns have been
    // shown
    while (possibleCount < 10 &&
           i < static_cast<int>(result.sortedGuesses.size())) {
      const auto &guess = result.sortedGuesses[i];
      i++;
      if (guess.probability <= 0.0) {
        continue; // Skip guesses with zero probability
      }
      possibleCount++;

      std::cout << std::setw(10) << (i);
      std::cout << std::setw(25) << guess.pattern.toString();
      std::cout << std::setw(12) << std::fixed << std::setprecision(3)
                << guess.ent;
      std::cout << std::setw(15) << std::fixed << std::setprecision(6)
                << guess.probability << "\n";
    }
  } else {
    std::cout << "No guesses available.\n";
  }
}

void MastermindGame::saveResults(const Mastermind::Result &result,
                                 const std::string &outputFile) {
  // Save to single output file
  std::filesystem::path outputPath(outputFile);
  if (!outputPath.parent_path().empty() &&
      !std::filesystem::exists(outputPath.parent_path())) {
    std::filesystem::create_directories(outputPath.parent_path());
  }

  std::ofstream out(outputFile);
  if (out.is_open()) {
    // Write possible patterns first (those with probability > 0)
    for (const auto &guess : result.sortedGuesses) {
      if (guess.probability > 0.0) {
        out << guess.pattern.toString() << "\n";
      }
    }
    for (const auto &guess : result.sortedGuesses) {
      out << guess.pattern.toString() << "," << guess.ent << ","
          << guess.probability << "\n";
    }
    out.close();
  } else {
    std::cerr << "Could not write to file: " << outputFile << "\n";
  }

  std::cout << result.totalPossiblePatterns << "\n";
  std::cout << result.sortedGuesses.size() << "\n";
  std::cout << outputFile;
}

void MastermindGame::runCLI() {
  Mastermind::Config config = getConfigFromUser();

  // Generate all possible patterns
  std::vector<Mastermind::Pattern> allPatterns =
      Mastermind::generateAllPatterns(config);
  std::cout << "Generated " << allPatterns.size() << " possible patterns.\n\n";

  std::vector<Mastermind::Feedback> guessHistory;

  while (true) {
    std::string input;
    try {
      std::cout << "\n=== MASTERMIND SOLVER ===\n";
      std::cout << "Commands: 's' (solve), 'c' (clear)\n";
      std::cout << "Format: PATTERN|FEEDBACK (e.g., '1 2 3 4|2 1')\n";
      std::cout << "  Feedback: <correct_position> <correct_color>\n\n";

      if (!guessHistory.empty()) {
        std::cout << "Current guess history:\n";
        for (size_t i = 0; i < guessHistory.size(); ++i) {
          std::cout << (i + 1) << ". " << guessHistory[i].guess.toString()
                    << " -> "
                    << static_cast<int>(guessHistory[i].correctPosition) << " "
                    << static_cast<int>(guessHistory[i].correctColor) << "\n";
        }
        std::cout << "\n";
      }

      std::cout << "Enter guess (or command): ";
      std::getline(std::cin, input);

      // Check for EOF
      if (std::cin.eof()) {
        std::cin.clear();
        std::cout << "\n";
        throw Utils::Input::UserCancelledException();
      }

      input = Utils::trimToLower(input);

      if (input.empty())
        continue;

      if (input == "c" || input == "clear") {
        guessHistory.clear();
        std::cout << "Guess history cleared.\n";
        continue;
      }

      if (input == "s" || input == "solve") {
        try {
          config.maxDepth =
              Utils::Input::promptInt("Enter search depth (0-2)", 1, 0, 2);

          std::cout << "Calculating best guesses...\n";
          Mastermind::Result result = Mastermind::runMastermindSolver(
              allPatterns, guessHistory, config);

          printResults(result);
          std::cout << "\nSolver completed.\n";
        } catch (const Utils::Input::UserCancelledException &) {
          std::cout << "Solve cancelled.\n";
        }
        continue;
      }
    } catch (const Utils::Input::UserCancelledException &) {
      // User pressed EOF at main input, return to game menu
      std::cout << "Returning to game menu...\n";
      return;
    }

    // Try to parse as pattern and feedback
    try {
      size_t pipePos = input.find('|');
      if (pipePos == std::string::npos) {
        std::cout << "Error: Missing '|' separator. Use format: 1 2 3 4|2 1\n";
        continue;
      }

      std::string patternStr = input.substr(0, pipePos);
      std::string feedbackStr = input.substr(pipePos + 1);

      // Parse pattern colors directly into Pattern
      std::istringstream patternIss(patternStr);
      Mastermind::Pattern pattern;
      pattern.numPegs = 0;
      std::string token;
      while (patternIss >> token && pattern.numPegs < Mastermind::MAX_PEGS) {
        int color = std::stoi(token);
        if (color < 0 || color > static_cast<int>(config.numColors)) {
          throw std::invalid_argument("Color out of range");
        }
        pattern.colors[pattern.numPegs] = static_cast<uint8_t>(color);
        pattern.numPegs++;
      }

      if (pattern.numPegs != config.numPegs) {
        std::cout << "Error: Pattern must have exactly " << config.numPegs
                  << " colors.\n";
        continue;
      }

      // Parse feedback
      std::istringstream feedbackIss(feedbackStr);
      int correctPos, correctCol;
      if (!(feedbackIss >> correctPos >> correctCol)) {
        std::cout << "Error: Invalid feedback format.\n";
        continue;
      }

      if (correctPos > static_cast<int>(config.numPegs) ||
          correctCol > static_cast<int>(config.numPegs) || correctPos < 0 ||
          correctCol < 0 ||
          (correctPos + correctCol) > static_cast<int>(config.numPegs)) {
        std::cout << "Error: Invalid feedback values.\n";
        continue;
      }
      Mastermind::Feedback feedback;
      feedback.guess = pattern;
      feedback.correctPosition = static_cast<uint8_t>(correctPos);
      feedback.correctColor = static_cast<uint8_t>(correctCol);

      guessHistory.push_back(feedback);
      std::cout << "Added guess: " << pattern.toString() << " with feedback "
                << correctPos << " " << correctCol << "\n\n";
    } catch (const std::exception &e) {
      std::cout << "Error: " << e.what() << "\n";
      std::cout << "Please use format: 1 2 3 4|2 1\n\n";
    }
  }
}

void MastermindGame::runHeadless(const Utils::Input::CommandArgs &cmdArgs) {
  try {
    const auto &args = cmdArgs.flags;
    Mastermind::Config config = getConfigFromArgs(args);
    std::vector<Mastermind::Feedback> guessHistory =
        getFeedbackFromArgs(args, config);

    // Generate all possible patterns
    std::vector<Mastermind::Pattern> allPatterns =
        Mastermind::generateAllPatterns(config);

    Mastermind::Result result =
        Mastermind::runMastermindSolver(allPatterns, guessHistory, config);

    std::string outputFile =
        Utils::Input::getArgValue(args, "o", std::string(""));
    if (outputFile.empty()) {
      outputFile = Utils::Input::getArgValue(
          args, "output", std::string("results/guesses.txt"));
    }

    saveResults(result, outputFile);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}

void MastermindGame::runGUI() {
  std::cout << "GUI mode not yet implemented for Mastermind.\n";
}

Utils::Testing::BenchmarkResult
MastermindGame::runBenchmark(const Utils::Testing::BenchmarkConfig &config) {
  Utils::Testing::BenchmarkResult result;
  result.gameMode = "mastermind";

  if (config.type == Utils::Testing::BenchmarkType::PERFORMANCE) {
    std::cout << "Performance benchmark is not available for Mastermind.\n";
    std::cout << "Running runtime benchmark instead.\n";
  }

  // Runtime benchmark
  result.iterations = config.iterations;

  if (config.verbose) {
    std::cout << "Running runtime benchmark for Mastermind ("
              << config.iterations << " iterations)...\n";
  }

  int64_t startTime = Utils::Profiling::getTime();

  for (int i = 0; i < config.iterations; ++i) {
    // Run a basic Mastermind solve with default configuration
    Mastermind::Config mastermindConfig;
    mastermindConfig.numPegs = 4;
    mastermindConfig.numColors = 6;
    mastermindConfig.allowDuplicates = true;
    mastermindConfig.maxDepth = 1;

    std::vector<Mastermind::Pattern> allPatterns =
        Mastermind::generateAllPatterns(mastermindConfig);
    std::vector<Mastermind::Feedback> emptyFeedback;

    Mastermind::runMastermindSolver(allPatterns, emptyFeedback,
                                    mastermindConfig);

    if (config.verbose && (i + 1) % std::max(1, config.iterations / 10) == 0) {
      std::cout << "Completed " << (i + 1) << "/" << config.iterations
                << " iterations\n";
    }
  }

  int64_t endTime = Utils::Profiling::getTime();

  result.totalTimeMs =
      (endTime - startTime) * Utils::Profiling::NANO_TO_SEC * 1000.0;
  result.averageTimeMs = result.totalTimeMs / config.iterations;

  return result;
}
} // namespace Game