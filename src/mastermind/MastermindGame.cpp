#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "mastermind/MastermindGame.hpp"
#include "utils/inputUtils.hpp"

namespace Game {
Mastermind::Config MastermindGame::getConfigFromUser() {
  Mastermind::Config config;

  std::cout << "=== Mastermind Solver ===\n";
  config.numPegs = static_cast<uint8_t>(
      Utils::Input::promptInt("Enter number of pegs", 4, 1, 20));

  std::cout
      << "Enter available color characters (e.g., 'rgbcmyk' or '012345'): ";
  std::getline(std::cin, config.colorChars);
  if (config.colorChars.empty()) {
    config.colorChars = "012345"; // Default
  }

  config.allowDuplicates =
      Utils::Input::promptBool("Allow duplicate colors?", true);

  return config;
}

Mastermind::Config MastermindGame::getConfigFromArgs(
    const std::map<std::string, std::string> &args) {
  Mastermind::Config config;
  config.numPegs =
      static_cast<uint8_t>(Utils::Input::getArgValue(args, "pegs", 4));
  config.colorChars =
      Utils::Input::getArgValue(args, "colors", std::string("012345"));
  config.allowDuplicates =
      Utils::Input::getArgValue(args, "allow-duplicates", true);
  config.maxDepth =
      static_cast<uint8_t>(Utils::Input::getArgValue(args, "max-depth", 1u));
  config.feedbackHistory = getFeedbackFromArgs(args, config);
  return config;
}

std::vector<Mastermind::Feedback>
MastermindGame::getFeedbackFromUser(const Mastermind::Config &config) {
  std::vector<Mastermind::Feedback> guessHistory;

  std::cout << "\nEnter your guesses and feedback.\n";
  std::cout << "Available colors: " << config.colorChars << "\n";
  std::cout << "Format: rgbc 2 1 (pattern correctPos correctCol)\n";
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
      // Use the parseFeedback function
      Mastermind::Feedback feedback = Mastermind::parseFeedback(input, config);
      guessHistory.push_back(feedback);
      std::cout << "Added: " << feedback.guess.toString(config)
                << " with feedback "
                << static_cast<int>(feedback.correctPosition) << " "
                << static_cast<int>(feedback.correctColor) << "\n";
    } catch (const std::exception &e) {
      std::cout << "Error parsing input: " << e.what() << "\n";
      std::cout << "Please use format: r g b c|2 1\n";
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
    if (trimmed.empty())
      continue;

    try {
      // Use the parseFeedback function
      Mastermind::Feedback feedback =
          Mastermind::parseFeedback(trimmed, config);
      guessHistory.push_back(feedback);
    } catch (const std::exception &e) {
      std::cerr << "Warning: Could not parse feedback '" << guessStr
                << "': " << e.what() << "\n";
    }
  }

  return guessHistory;
}

void MastermindGame::printResults(const Mastermind::Result &result,
                                  const Mastermind::Config &config) {
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
      std::cout << std::setw(25) << guess.pattern.toString(config);
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
      std::cout << std::setw(25) << guess.pattern.toString(config);
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
                                 const std::string &outputFile,
                                 const Mastermind::Config &config) {
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
        out << guess.pattern.toString(config) << "\n";
      }
    }
    for (const auto &guess : result.sortedGuesses) {
      out << guess.pattern.toString(config) << "," << guess.ent << ","
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

  while (true) {
    std::string input;
    try {
      std::cout << "\n=== MASTERMIND SOLVER ===\n";
      std::cout << "Commands: 's' (solve), 'c' (clear)\n";
      std::cout << "Available colors: " << config.colorChars << "\n";
      std::cout << "Format: PATTERN POS COL (e.g., 'rgbc 2 1')\n";
      std::cout << "  Feedback: <correct_position> <correct_color>\n\n";

      if (!config.feedbackHistory.empty()) {
        std::cout << "Current guess history:\n";
        for (size_t i = 0; i < config.feedbackHistory.size(); ++i) {
          std::cout << (i + 1) << ". "
                    << config.feedbackHistory[i].guess.toString(config)
                    << " -> "
                    << static_cast<int>(
                           config.feedbackHistory[i].correctPosition)
                    << " "
                    << static_cast<int>(config.feedbackHistory[i].correctColor)
                    << "\n";
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
        config.feedbackHistory.clear();
        std::cout << "Guess history cleared.\n";
        continue;
      }

      if (input == "s" || input == "solve") {
        try {
          config.maxDepth = static_cast<uint8_t>(
              Utils::Input::promptInt("Enter search depth (0-2)", 1, 0, 2));

          std::cout << "Calculating best guesses...\n";
          Mastermind::Result result = Mastermind::runMastermindSolver(config);

          printResults(result, config);
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
      // Use the parseFeedback function
      Mastermind::Feedback feedback = Mastermind::parseFeedback(input, config);
      config.feedbackHistory.push_back(feedback);
      std::cout << "Added guess: " << feedback.guess.toString(config)
                << " with feedback "
                << static_cast<int>(feedback.correctPosition) << " "
                << static_cast<int>(feedback.correctColor) << "\n\n";
    } catch (const std::exception &e) {
      std::cout << "Error: " << e.what() << "\n";
      std::cout << "Please use format: rgbc 2 1\n\n";
    }
  }
}

void MastermindGame::runHeadless(const Utils::Input::CommandArgs &cmdArgs) {
  try {
    const auto &args = cmdArgs.flags;
    Mastermind::Config config = getConfigFromArgs(args);

    Mastermind::Result result = Mastermind::runMastermindSolver(config);

    std::string outputFile =
        Utils::Input::getArgValue(args, "o", std::string(""));
    if (outputFile.empty()) {
      outputFile = Utils::Input::getArgValue(
          args, "output", std::string("results/guesses.txt"));
    }

    saveResults(result, outputFile, config);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}

void MastermindGame::runGUI() {
  std::cout << "GUI mode not yet implemented for Mastermind.\n";
}
} // namespace Game