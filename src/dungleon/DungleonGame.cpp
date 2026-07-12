#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "dungleon/DungleonGame.hpp"
#include "utils/inputUtils.hpp"

namespace Game {

Dungleon::Config DungleonGame::getConfigFromUser() {
  Dungleon::Config config;
  std::cout << "Configure Dungleon solver:\n";
  config.autoDepth = Utils::Input::promptBool(
      "Use auto-depth calculation (Recommended)?", true);
  if (!config.autoDepth) {
    config.maxDepth = static_cast<uint8_t>(Utils::Input::promptInt(
        "Search depth for ENT calculation (0-2)", 1, 0, 2));
  }
  config.maxGuesses = Utils::Input::promptInt(
      "Maximum number of guesses allowed", 10, 1, 100);
  return config;
}

Dungleon::Config DungleonGame::getConfigFromArgs(
    const std::map<std::string, std::string> &args) {
  Dungleon::Config config;
  config.maxDepth =
      static_cast<uint8_t>(Utils::Input::getArgValue(args, "max-depth", 0));
  config.autoDepth = Utils::Input::getArgValue(args, "auto-depth", false);
  config.maxGuesses = Utils::Input::getArgValue(args, "max-guesses", 10);
  config.excludeImpossiblePatterns =
      Utils::Input::getArgValue(args, "exclude-impossible", false);
  return config;
}

std::vector<Dungleon::Feedback> DungleonGame::getFeedbackFromArgs(
    const std::map<std::string, std::string> &args) {
  std::vector<Dungleon::Feedback> feedbackHistory;

  auto it = args.find("guesses");
  if (it == args.end())
    return feedbackHistory;

  std::istringstream iss(it->second);
  std::string guessStr;
  while (std::getline(iss, guessStr, ';')) {
    std::string trimmed = Utils::trimToLower(guessStr);
    if (trimmed.empty())
      continue;
    try {
      Dungleon::Feedback fb =
          Dungleon::parseFeedback(trimmed, Dungleon::Config{});
      feedbackHistory.push_back(fb);
    } catch (const std::exception &e) {
      std::cerr << "Warning: Could not parse feedback '" << guessStr
                << "': " << e.what() << "\n";
    }
  }

  return feedbackHistory;
}

Dungleon::Pattern DungleonGame::parsePattern(const std::string &input) {
  std::istringstream iss(input);
  std::vector<std::string> tokens;
  std::string token;
  while (iss >> token) {
    tokens.push_back(token);
  }

  if (tokens.size() != 5) {
    throw std::runtime_error(
        "Invalid pattern format. Expected 5 character pairs.");
  }

  std::array<uint8_t, 5> characters = {};
  for (size_t i = 0; i < 5; ++i) {
    const std::string &charPair = tokens[i];
    if (charPair.length() != 2) {
      throw std::runtime_error(
          "Invalid character pair '" + charPair +
          "'. Each character must be exactly 2 characters.");
    }

    bool found = false;
    for (uint8_t j = 0; j < Dungleon::NUM_CHARACTERS; ++j) {
      if (Dungleon::CHARACTER_IDS[j] == charPair) {
        characters[i] = j;
        found = true;
        break;
      }
    }

    if (!found) {
      throw std::runtime_error("Unknown character '" + charPair + "'");
    }
  }

  return Dungleon::Pattern(characters);
}

std::vector<Dungleon::Pattern> DungleonGame::getSolutionsFromArgs(
    const std::map<std::string, std::string> &args) {
  std::vector<Dungleon::Pattern> solutionHistory;

  auto it = args.find("solutions");
  if (it == args.end())
    return solutionHistory;

  std::istringstream iss(it->second);
  std::string solutionStr;
  while (std::getline(iss, solutionStr, ';')) {
    std::string trimmed = Utils::trimToLower(solutionStr);
    if (trimmed.empty())
      continue;
    try {
      Dungleon::Pattern pattern = parsePattern(trimmed);
      solutionHistory.push_back(pattern);
    } catch (const std::exception &e) {
      std::cerr << "Warning: Could not parse solution '" << solutionStr
                << "': " << e.what() << "\n";
    }
  }

  return solutionHistory;
}

void DungleonGame::printResults(const Dungleon::Result &result) {
  std::cout << "\n=== SOLVER RESULTS ===\n";

  if (result.totalPossiblePatterns == 0) {
    std::cout << "No possible patterns found with given constraints.\n";
    return;
  }

  std::cout << "Possible patterns remaining: " << result.totalPossiblePatterns
            << "\n";
  std::cout << "Search depth used: " << result.searchDepth << "\n\n";

  if (!result.sortedGuesses.empty()) {
    std::cout << "=== Best guesses ===\n";

    // Header
    std::cout << std::setw(6) << "Rank";
    std::cout << std::setw(20) << "Pattern";
    std::cout << std::setw(12) << "ENT Score";
    std::cout << std::setw(12) << "WNT Score";
    std::cout << std::setw(15) << "Probability" << "\n";
    std::cout << std::string(65, '-') << "\n";

    int possibleCount = 0;
    int i = 0;

    // Print the top 10 guesses first
    while (i < 10 && i < static_cast<int>(result.sortedGuesses.size())) {
      const auto &guess = result.sortedGuesses[i];
      if (guess.probability > 0.0)
        possibleCount++;
      i++;

      std::cout << std::setw(6) << i;
      std::cout << std::setw(20) << guess.pattern.toString();
      std::cout << std::setw(12) << std::fixed << std::setprecision(3)
                << guess.ent;
      std::cout << std::setw(12) << std::fixed << std::setprecision(3)
                << guess.wnt;
      std::cout << std::setw(15) << std::fixed << std::setprecision(6)
                << guess.probability << "\n";
    }

    std::cout << std::string(65, '-') << "\n";

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

      std::cout << std::setw(6) << i;
      std::cout << std::setw(20) << guess.pattern.toString();
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

void DungleonGame::saveResults(const Dungleon::Result &result,
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
        out << guess.pattern.toString() << "," << guess.ent << "," << guess.wnt << ","
            << guess.probability << "\n";
      }
    }
    for (const auto &guess : result.sortedGuesses) {
      out << guess.pattern.toString() << "," << guess.ent << "," << guess.wnt << ","
          << guess.probability << "\n";
    }
    out.close();
  } else {
    std::cerr << "Could not write to file: " << outputFile << "\n";
  }

  std::cout << result.totalPossiblePatterns << "\n";
  std::cout << result.sortedGuesses.size() << "\n";
  std::cout << outputFile << "\n";
  std::cout << result.searchDepth;
}

void DungleonGame::runCLI() {
  std::vector<Dungleon::Feedback> feedbackHistory;
  Dungleon::Config config; // defer asking for depth until user requests a solve

  while (true) {
    try {
      std::cout << "\n=== DUNGLEON SOLVER ===\n";
      std::cout << "Commands: 's' (solve), 'c' (clear), 'config' (change)\n";
      std::cout << "Format: 'ar kn ma bt dr 01234' (colors 0-4)\n";
      std::cout
          << "        'ar kn ma bt dr' (past solution for Gauntlet mode)\n";
      std::cout << "Colors: 0=not present, 1=diff pos no more, 2=diff pos "
                   "one more,\n";
      std::cout << "        3=correct pos no more, 4=correct pos one more\n\n";

      if (!config.solutionHistory.empty()) {
        std::cout << "Past solutions (Gauntlet mode):\n";
        for (const auto &pattern : config.solutionHistory) {
          std::cout << "  " << pattern.toString() << "\n";
        }
        std::cout << "\n";
      }

      if (!feedbackHistory.empty()) {
        std::cout << "Current feedback history:\n";
        for (const auto &fb : feedbackHistory) {
          std::cout << "  " << fb.pattern.toString() << " -> ";
          for (size_t i = 0; i < 5; ++i)
            std::cout << static_cast<int>(fb.getColor(i));
          std::cout << "\n";
        }
        std::cout << "\n";
      }

      std::cout << "Enter guess (or command): ";
      std::string input = Utils::Input::readLine();

      input = Utils::trimToLower(input);
      if (input.empty())
        continue;
      if (input == "c" || input == "clear") {
        feedbackHistory.clear();
        config.solutionHistory.clear();
        std::cout << "Feedback history and past solutions cleared.\n";
        continue;
      }
      if (input == "config" || input == "reconfigure") {
        config = getConfigFromUser();
        if (!feedbackHistory.empty() || !config.solutionHistory.empty()) {
          feedbackHistory.clear();
          config.solutionHistory.clear();
          std::cout << "Feedback history and past solutions cleared due to "
                       "config change.\n";
        }
        continue;
      }
      if (input == "s" || input == "solve") {
        try {
          // Ask for search depth each time before computing ENT
          config.autoDepth = Utils::Input::promptBool(
              "Use auto-depth calculation (Recommended)?", true);
          if (!config.autoDepth) {
            config.maxDepth = static_cast<uint8_t>(Utils::Input::promptInt(
                "Search depth for ENT calculation (0-2)", 1, 0, 2));
          }
          config.excludeImpossiblePatterns = Utils::Input::promptBool(
              "Exclude impossible patterns from guesses?", false);

          std::cout << "Calculating best guesses...\n";
          config.feedbackHistory = feedbackHistory;
          Dungleon::Result result = Dungleon::runDungleonSolver(config);
          printResults(result);
        } catch (const Utils::Input::UserCancelledException &) {
          std::cout << "Solve cancelled.\n";
        }
        continue;
      }

      try {
        // Try to parse as full feedback first
        Dungleon::Feedback fb = Dungleon::parseFeedback(input, config);
        feedbackHistory.push_back(fb);
        config.feedbackHistory = feedbackHistory;
        std::cout << "Added feedback for " << fb.pattern.toString() << "\n";
      } catch (const std::exception &e) {
        // If that fails, try to parse as a pattern-only input (past solution)
        try {
          Dungleon::Pattern pattern = parsePattern(input);
          config.solutionHistory.push_back(pattern);
          std::cout << "Added past solution (Gauntlet mode): "
                    << pattern.toString() << "\n";
        } catch (const std::exception &ex) {
          std::cout << "Error: " << e.what() << "\n";
          std::cout << "Use format: 'ar kn ma bt dr 01234' (with feedback) or "
                       "'ar kn ma bt dr' (past solution)\n";
        }
      }

    } catch (const Utils::Input::UserCancelledException &) {
      std::cout << "Returning to game menu...\n";
      return;
    }
  }
}

void DungleonGame::runHeadless(const Utils::Input::CommandArgs &cmdArgs) {
  try {
    const auto &args = cmdArgs.flags;
    Dungleon::Config config = getConfigFromArgs(args);

    // Load feedback history and solution history from args
    config.feedbackHistory = getFeedbackFromArgs(args);
    config.solutionHistory = getSolutionsFromArgs(args);

    Dungleon::Result result = Dungleon::runDungleonSolver(config);

    std::string outputFile =
        Utils::Input::getArgValue(args, "o", std::string(""));
    if (outputFile.empty()) {
      outputFile = Utils::Input::getArgValue(
          args, "output", std::string("results/dungleon.txt"));
    }

    saveResults(result, outputFile);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}

} // namespace Game
