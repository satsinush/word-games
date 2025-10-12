#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "../utils/inputUtils.hpp"
#include "WordleGame.hpp"

namespace Game {
WordleGame::WordleGame(const std::vector<Utils::Word> &words)
    : wordVec(words) {}

Wordle::Config WordleGame::getConfigFromUser() {
  Wordle::Config config;

  std::cout << "Configure Wordle solver:\n";
  config.maxDepth = Utils::Input::promptInt(
      "Search depth for entropy calculation (0-2)", 0, 0, 2);
  config.excludeUncommonWords = Utils::Input::promptBool(
      "Exclude uncommon words from suggestions?", false);

  return config;
}

Wordle::Config
WordleGame::getConfigFromArgs(const std::map<std::string, std::string> &args) {
  Wordle::Config config;
  config.maxDepth = Utils::Input::getArgValue(args, "maxDepth", 0);
  config.excludeUncommonWords =
      Utils::Input::getArgValue(args, "excludeUncommonWords", false);
  return config;
}

std::vector<Wordle::Feedback> WordleGame::getFeedbackFromUser() {
  std::vector<Wordle::Feedback> feedbackHistory;

  std::cout << "\n=== WORDLE SOLVER ===\n";
  std::cout << "Enter your guesses and their feedback patterns.\n";
  std::cout << "Format: WORD 01201 (0=grey, 1=yellow, 2=green)\n";
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
      Wordle::Feedback fb = Wordle::parseFeedback(input);
      feedbackHistory.push_back(fb);
      std::cout << "Added: " << fb.word << " with pattern ";
      for (int i = 0; i < 5; ++i) {
        std::cout << fb.getColor(i);
      }
      std::cout << "\n";
    } catch (const std::exception &e) {
      std::cout << "Error parsing feedback: " << e.what() << "\n";
      std::cout << "Please use format: WORD 01201\n";
    }
  }

  return feedbackHistory;
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
    std::cout << "Best next guesses (top 10):\n";
    std::cout << std::setw(12) << "Word" << std::setw(12) << "Entropy"
              << std::setw(15) << "Probability" << "\n";
    std::cout << std::string(40, '-') << "\n";

    int count = 0;
    for (const auto &guess : result.sortedGuesses) {
      if (++count > 10)
        break;

      std::cout << std::setw(12) << guess.word.wordString << std::setw(12)
                << std::fixed << std::setprecision(3) << guess.entropy
                << std::setw(15) << std::fixed << std::setprecision(6)
                << guess.probability << "\n";
    }
  }
}

void WordleGame::saveResults(const Wordle::Result &result,
                             const std::string &possibleFile,
                             const std::string &guessesFile) {
  // Save possible words
  std::filesystem::path possiblePath(possibleFile);
  if (!possiblePath.parent_path().empty() &&
      !std::filesystem::exists(possiblePath.parent_path())) {
    std::filesystem::create_directories(possiblePath.parent_path());
  }

  std::ofstream possibleOut(possibleFile);
  if (possibleOut.is_open()) {
    // Extract possible words from sorted guesses that have probability > 0
    for (const auto &guess : result.sortedGuesses) {
      if (guess.probability > 0.0) {
        possibleOut << guess.word.wordString << "\n";
      }
    }
    possibleOut.close();
  }

  // Save all guesses with entropy
  std::filesystem::path guessesPath(guessesFile);
  if (!guessesPath.parent_path().empty() &&
      !std::filesystem::exists(guessesPath.parent_path())) {
    std::filesystem::create_directories(guessesPath.parent_path());
  }

  std::ofstream guessesOut(guessesFile);
  if (guessesOut.is_open()) {
    guessesOut << "word,entropy,probability\n";
    for (const auto &guess : result.sortedGuesses) {
      guessesOut << guess.word.wordString << "," << guess.entropy << ","
                 << guess.probability << "\n";
    }
    guessesOut.close();
  }

  std::cout << result.totalPossibleWords << "\n";
  std::cout << result.sortedGuesses.size() << "\n";
  std::cout << possibleFile << "\n";
  std::cout << guessesFile;
}

void WordleGame::runCLI() {
  std::vector<Wordle::Feedback> feedbackHistory;

  while (true) {
    std::cout << "\n=== WORDLE SOLVER ===\n";
    std::cout << "Enter your guesses and their feedback patterns.\n";
    std::cout << "Format: WORD 01201 (0=grey, 1=yellow, 2=green)\n";
    std::cout << "Enter 'solve' to get best guesses, 'clear' to start over, "
                 "'q' to quit\n\n";

    if (!feedbackHistory.empty()) {
      std::cout << "Current feedback history:\n";
      for (const auto &fb : feedbackHistory) {
        std::cout << "  " << fb.word << " -> ";
        for (int i = 0; i < 5; ++i) {
          std::cout << fb.getColor(i);
        }
        std::cout << "\n";
      }
      std::cout << "\n";
    }

    while (true) {
      std::cout << "Enter guess (or command): ";
      std::string input;
      std::getline(std::cin, input);
      input = Utils::trimToLower(input);

      if (input.empty())
        continue;
      if (input == "q")
        return;

      if (input == "clear") {
        feedbackHistory.clear();
        std::cout << "Feedback history cleared.\n\n";
        break;
      }

      if (input == "solve") {
        Wordle::Config config = getConfigFromUser();

        std::cout << "Calculating best guesses...\n";
        Wordle::Result result =
            Wordle::runWordleSolver(wordVec, feedbackHistory, config);

        printResults(result);
        continue;
      }

      // Try to parse as feedback
      try {
        Wordle::Feedback fb = Wordle::parseFeedback(input);
        feedbackHistory.push_back(fb);
        std::cout << "Added feedback for " << fb.word << "\n";
      } catch (const std::exception &e) {
        std::cout << "Error: " << e.what() << "\n";
        std::cout << "Use format: WORD 01201 or commands: solve, clear, q\n";
      }
    }
  }
}

void WordleGame::runHeadless(const std::map<std::string, std::string> &args) {
  try {
    Wordle::Config config = getConfigFromArgs(args);
    std::vector<Wordle::Feedback> feedbackHistory = getFeedbackFromArgs(args);

    Wordle::Result result =
        Wordle::runWordleSolver(wordVec, feedbackHistory, config);

    std::string possibleFile = Utils::Input::getArgValue(
        args, "possibleFile", std::string("results/possible.txt"));
    std::string guessesFile = Utils::Input::getArgValue(
        args, "guessesFile", std::string("results/guesses.txt"));

    saveResults(result, possibleFile, guessesFile);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}

void WordleGame::runGUI() {
  std::cout << "GUI mode not yet implemented for Wordle.\n";
}
} // namespace Game