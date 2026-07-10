#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "utils/inputUtils.hpp"
#include "wordle/WordleGame.hpp"

namespace Game {
WordleGame::WordleGame() {}

Wordle::Config WordleGame::getConfigFromUser() {
  Wordle::Config config;

  std::cout << "Configure Wordle solver:\n";
  config.wordLength = Utils::Input::promptInt("Word length (1-32)", 5, 1, 32);
  config.autoDepth = Utils::Input::promptBool(
      "Use auto-depth calculation (Recommended)?", true);
  if (!config.autoDepth) {
    config.maxDepth = Utils::Input::promptInt(
        "Search depth for ENT calculation (0-2)", 1, 0, 2);
  }
  config.excludeUncommonWords = Utils::Input::promptBool(
      "Exclude uncommon words from suggestions?", true);
  config.maxGuesses = Utils::Input::promptInt(
      "Maximum number of guesses allowed", 6, 1, 100);

  return config;
}

Wordle::Config
WordleGame::getConfigFromArgs(const std::map<std::string, std::string> &args) {
  Wordle::Config config;
  config.wordLength = Utils::Input::getArgValue(args, "word-length", 5);
  config.maxDepth = Utils::Input::getArgValue(args, "max-depth", 0);
  config.autoDepth = Utils::Input::getArgValue(args, "auto-depth", false);
  config.excludeUncommonWords =
      Utils::Input::getArgValue(args, "exclude-uncommon-words", false);
  config.maxGuesses = Utils::Input::getArgValue(args, "max-guesses", 6);
  config.feedbackHistory =
      getFeedbackFromArgs(args); // Get feedback from args if provided
  return config;
}

std::vector<Wordle::Feedback> WordleGame::getFeedbackFromArgs(
    const std::map<std::string, std::string> &args) {
  std::vector<Wordle::Feedback> feedbackHistory;

  // Look for guesses argument
  auto it = args.find("guesses");
  if (it == args.end()) {
    return feedbackHistory; // Return empty if no guesses provided
  }

  // Parse multiple feedback strings in format: "STEAL 20100;CRANE 01002"
  std::istringstream iss(it->second);
  std::string guessStr;

  // Parse each guess separated by semicolons
  while (std::getline(iss, guessStr, ';')) {
    std::string trimmed = Utils::trimToLower(guessStr);
    if (trimmed.empty() || trimmed.find(' ') == std::string::npos)
      continue;

    try {
      Wordle::Feedback fb = Wordle::parseFeedback(trimmed);
      feedbackHistory.push_back(fb);
    } catch (const std::exception &e) {
      std::cerr << "Warning: Could not parse feedback '" << guessStr
                << "': " << e.what() << "\n";
    }
  }

  return feedbackHistory;
}

void WordleGame::printResults(const Wordle::Result &result) {
  std::cout << "\n=== SOLVER RESULTS ===\n";

  if (result.totalPossibleWords == 0) {
    std::cout << "No possible words found with given constraints.\n";
    return;
  }

  std::cout << "Possible words remaining: " << result.totalPossibleWords
            << "\n\n";

  if (!result.sortedGuesses.empty()) {
    std::cout << "=== Best guesses ===\n";

    // Header
    std::cout << std::setw(10) << "Rank";
    std::cout << std::setw(12) << "Word";
    std::cout << std::setw(12) << "Word Score";
    std::cout << std::setw(12) << "ENT Score";
    std::cout << std::setw(12) << "WNT Score";
    std::cout << std::setw(15) << "Probability" << "\n";

    int totalWidth = 10 + 12 + 12 + 12 + 12 + 15;
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
      std::cout << std::setw(12) << guess.word.wordString;
      std::cout << std::setw(12) << std::fixed << std::setprecision(3)
                << guess.word.score;
      std::cout << std::setw(12) << std::fixed << std::setprecision(3)
                << guess.ent;
      std::cout << std::setw(12) << std::fixed << std::setprecision(3)
                << guess.wnt;
      std::cout << std::setw(15) << std::fixed << std::setprecision(6)
                << guess.probability << "\n";
    }

    std::cout << std::string(std::max(0, totalWidth), '-') << "\n";

    // Print the next possible guesses until 10 possible words have been shown
    while (possibleCount < 10 &&
           i < static_cast<int>(result.sortedGuesses.size())) {
      const auto &guess = result.sortedGuesses[i];
      i++;
      if (guess.probability <= 0.0) {
        continue; // Skip guesses with zero probability
      }
      possibleCount++;

      std::cout << std::setw(10) << (i);
      std::cout << std::setw(12) << guess.word.wordString;
      std::cout << std::setw(12) << std::fixed << std::setprecision(3)
                << guess.word.score;
      std::cout << std::setw(12) << std::fixed << std::setprecision(3)
                << guess.ent;
      std::cout << std::setw(12) << std::fixed << std::setprecision(3)
                << guess.wnt;
      std::cout << std::setw(15) << std::fixed << std::setprecision(6)
                << guess.probability << "\n";
    }
  } else {
    std::cout << "No guesses available.\n";
  }
}

void WordleGame::saveResults(const Wordle::Result &result,
                             const std::string &outputFile) {
  // Save to single output file
  std::filesystem::path outputPath(outputFile);
  if (!outputPath.parent_path().empty() &&
      !std::filesystem::exists(outputPath.parent_path())) {
    std::filesystem::create_directories(outputPath.parent_path());
  }

  std::ofstream out(outputFile);
  if (out.is_open()) {
    // Write possible words first (those with probability > 0)
    for (const auto &guess : result.sortedGuesses) {
      if (guess.probability > 0.0) {
        out << guess.word.wordString << "," << guess.ent << "," << guess.wnt << ","
            << guess.probability << "\n";
      }
    }
    for (const auto &guess : result.sortedGuesses) {
      out << guess.word.wordString << "," << guess.ent << "," << guess.wnt << ","
          << guess.probability << "\n";
    }
    out.close();
  } else {
    std::cerr << "Could not write to file: " << outputFile << "\n";
  }

  std::cout << result.totalPossibleWords << "\n";
  std::cout << result.sortedGuesses.size() << "\n";
  std::cout << outputFile << "\n";
  std::cout << result.searchDepth;
}

void WordleGame::runCLI() {
  Wordle::Config config;

  // Get word length configuration upfront
  std::cout << "\n=== WORDLE SOLVER SETUP ===\n";
  config.wordLength = Utils::Input::promptInt("Word length (1-32)", 5, 1, 32);

  std::cout << "\nWord length set to: " << static_cast<int>(config.wordLength)
            << " letters\n";

  while (true) {
    try {
      std::cout << "\n=== WORDLE SOLVER ===\n";
      std::cout << "Commands: 's' (solve), 'c' (clear), 'config' (change word "
                   "length)\n";
      std::cout << "Format: WORD " << std::string(config.wordLength, '0')
                << " (0=grey, 1=yellow, 2=green)\n";
      std::cout << "Word and pattern must be "
                << static_cast<int>(config.wordLength) << " characters.\n\n";

      if (!config.feedbackHistory.empty()) {
        std::cout << "Current feedback history:\n";
        for (const auto &fb : config.feedbackHistory) {
          std::cout << "  " << fb.word << " -> ";
          for (size_t i = 0; i < fb.word.size(); ++i) {
            std::cout << fb.getColor(i);
          }
          std::cout << "\n";
        }
        std::cout << "\n";
      }

      std::cout << "Enter guess (or command): ";
      std::string input;
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
        std::cout << "Feedback history cleared.\n";
        continue;
      }

      if (input == "config" || input == "reconfigure") {
        std::cout << "\n=== CHANGE WORD LENGTH ===\n";
        config.wordLength = Utils::Input::promptInt("Word length (1-32)",
                                                    config.wordLength, 1, 32);

        std::cout << "\nWord length updated to: "
                  << static_cast<int>(config.wordLength) << " letters\n";

        // Clear feedback history when changing word length
        if (!config.feedbackHistory.empty()) {
          config.feedbackHistory.clear();
          std::cout << "Feedback history cleared due to word length change.\n";
        }
        continue;
      }

      if (input == "s" || input == "solve") {
        try {
          // Ask for solver options each time
          config.autoDepth = Utils::Input::promptBool(
              "Use auto-depth calculation (Recommended)?", true);
          if (!config.autoDepth) {
            config.maxDepth = Utils::Input::promptInt(
                "Search depth for ENT calculation (0-2)", 1, 0, 2);
          }
          config.excludeUncommonWords = Utils::Input::promptBool(
              "Exclude uncommon words from suggestions?", true);

          std::cout << "Calculating best guesses...\n";
          Wordle::Result result = Wordle::runWordleSolver(config);

          printResults(result);
        } catch (const Utils::Input::UserCancelledException &) {
          std::cout << "Solve cancelled.\n";
        }
        continue;
      }

      // Try to parse as feedback
      try {
        Wordle::Feedback fb = Wordle::parseFeedback(input);

        // Validate word length matches config
        if (fb.word.size() != config.wordLength) {
          std::cout << "Error: Word must be exactly "
                    << static_cast<int>(config.wordLength) << " letters (got "
                    << fb.word.size() << ").\n";
          continue;
        }

        config.feedbackHistory.push_back(fb);
        std::cout << "Added feedback for " << fb.word << "\n";
      } catch (const std::exception &e) {
        std::cout << "Error: " << e.what() << "\n";
        std::cout << "Use format: WORD " << std::string(config.wordLength, '0')
                  << " or commands: s, c, config\n";
      }
    } catch (const Utils::Input::UserCancelledException &) {
      // User pressed EOF at main input, return to game menu
      std::cout << "Returning to game menu...\n";
      return;
    }
  }
}

void WordleGame::runHeadless(const Utils::Input::CommandArgs &cmdArgs) {
  try {
    const auto &args = cmdArgs.flags;
    Wordle::Config config = getConfigFromArgs(args);

    Wordle::Result result = Wordle::runWordleSolver(config);

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
} // namespace Game