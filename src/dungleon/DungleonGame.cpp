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
  config.maxDepth = static_cast<uint8_t>(Utils::Input::promptInt(
      "Search depth for ENT calculation (0-2)", 1, 0, 2));
  return config;
}

Dungleon::Config DungleonGame::getConfigFromArgs(
    const std::map<std::string, std::string> &args) {
  Dungleon::Config config;
  config.maxDepth =
      static_cast<uint8_t>(Utils::Input::getArgValue(args, "max-depth", 0));
  return config;
}

std::vector<Dungleon::Feedback> DungleonGame::getFeedbackFromUser() {
  std::vector<Dungleon::Feedback> feedbackHistory;

  std::cout << "\n=== DUNGLEON SOLVER ===\n";
  std::cout << "Enter your guesses and their feedback patterns.\n";
  std::cout
      << "Format: 'ar kn ma bt dr 01234' (5 two-letter ids and 5 digits 0-4)\n";
  std::cout
      << "Colors: 0=not present, 1=diff pos no more, 2=correct pos no more,\n";
  std::cout << "        3=diff pos one more, 4=correct pos one more\n";
  std::cout << "Enter 'done' when finished entering feedback.\n\n";

  while (true) {
    std::cout << "Enter guess and feedback (or 'done'): ";
    std::string input;
    std::getline(std::cin, input);
    input = Utils::trimToLower(input);

    if (input.empty())
      continue;
    if (input == "done")
      break;

    try {
      Dungleon::Feedback fb =
          Dungleon::parseFeedback(input, Dungleon::Config{});
      feedbackHistory.push_back(fb);
      std::cout << "Added: " << fb.pattern.toString() << " with pattern ";
      for (size_t i = 0; i < 5; ++i) {
        std::cout << fb.getColor(i);
      }
      std::cout << "\n";
    } catch (const std::exception &e) {
      std::cout << "Error parsing feedback: " << e.what() << "\n";
      std::cout << "Please use format: 'ar kn ma bt dr 01234'\n";
    }
  }

  return feedbackHistory;
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

void DungleonGame::printResults(const Dungleon::Result &result) {
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
    std::cout << std::setw(6) << "Rank";
    std::cout << std::setw(20) << "Pattern";
    std::cout << std::setw(12) << "ENT Score";
    std::cout << std::setw(15) << "Probability" << "\n";
    std::cout << std::string(53, '-') << "\n";

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
      std::cout << std::setw(15) << std::fixed << std::setprecision(6)
                << guess.probability << "\n";
    }

    std::cout << std::string(53, '-') << "\n";

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
      std::cout << std::setw(15) << std::fixed << std::setprecision(6)
                << guess.probability << "\n";
    }
  } else {
    std::cout << "No guesses available.\n";
  }
}

void DungleonGame::saveResults(const Dungleon::Result &result,
                               const std::string &outputFile) {
  std::filesystem::path outputPath(outputFile);
  if (!outputPath.parent_path().empty() &&
      !std::filesystem::exists(outputPath.parent_path())) {
    std::filesystem::create_directories(outputPath.parent_path());
  }

  std::ofstream out(outputFile);
  if (!out.is_open()) {
    std::cerr << "Could not write to file: " << outputFile << "\n";
    return;
  }

  for (const auto &g : result.sortedGuesses) {
    out << g.pattern.toString() << "," << g.ent << "," << g.probability << "\n";
  }

  out.close();
}

void DungleonGame::runCLI() {
  std::vector<Dungleon::Feedback> feedbackHistory;
  Dungleon::Config config; // defer asking for depth until user requests a solve

  while (true) {
    try {
      std::cout << "\n=== DUNGLEON SOLVER ===\n";
      std::cout << "Commands: 's' (solve), 'c' (clear), 'config' (change)\n";
      std::cout << "Format: 'ar kn ma bt dr 01234' (colors 0-4)\n";
      std::cout << "Colors: 0=not present, 1=diff pos no more, 2=correct pos "
                   "no more,\n";
      std::cout << "        3=diff pos one more, 4=correct pos one more\n\n";

      if (!feedbackHistory.empty()) {
        std::cout << "Current feedback history:\n";
        for (const auto &fb : feedbackHistory) {
          std::cout << "  " << fb.pattern.toString() << " -> ";
          for (size_t i = 0; i < 5; ++i)
            std::cout << fb.getColor(i);
          std::cout << "\n";
        }
        std::cout << "\n";
      }

      std::cout << "Enter guess (or command): ";
      std::string input;
      std::getline(std::cin, input);

      if (std::cin.eof()) {
        std::cin.clear();
        throw Utils::Input::UserCancelledException();
      }

      input = Utils::trimToLower(input);
      if (input.empty())
        continue;
      if (input == "c" || input == "clear") {
        feedbackHistory.clear();
        std::cout << "Feedback history cleared.\n";
        continue;
      }
      if (input == "config" || input == "reconfigure") {
        config = getConfigFromUser();
        if (!feedbackHistory.empty()) {
          feedbackHistory.clear();
          std::cout << "Feedback history cleared due to config change.\n";
        }
        continue;
      }
      if (input == "s" || input == "solve") {
        try {
          // Ask for search depth each time before computing ENT
          config.maxDepth = static_cast<uint8_t>(Utils::Input::promptInt(
              "Search depth for ENT calculation (0-2)", 1, 0, 2));

          std::cout << "Calculating best guesses...\n";
          // Use all patterns for guesses (including invalid patterns)
          std::vector<Dungleon::Pattern> allPatterns =
              Dungleon::generateAllPossiblePatterns();
          Dungleon::Result result = Dungleon::runDungleonSolver(config);
          printResults(result);
        } catch (const Utils::Input::UserCancelledException &) {
          std::cout << "Solve cancelled.\n";
        }
        continue;
      }

      try {
        Dungleon::Feedback fb = Dungleon::parseFeedback(input, config);
        feedbackHistory.push_back(fb);
        std::cout << "Added feedback for " << fb.pattern.toString() << "\n";
      } catch (const std::exception &e) {
        std::cout << "Error: " << e.what() << "\n";
        std::cout
            << "Use format: 'ar kn ma bt dr 01234' or commands: s, c, config\n";
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
    std::vector<Dungleon::Feedback> feedbackHistory = getFeedbackFromArgs(args);

    // Use all patterns for guesses (including invalid patterns)
    std::vector<Dungleon::Pattern> allPatterns =
        Dungleon::generateAllPossiblePatterns();
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

void DungleonGame::runGUI() {
  std::cout << "GUI mode not implemented for Dungleon.\n";
}

} // namespace Game
