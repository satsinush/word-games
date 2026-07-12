#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "hangman/HangmanGame.hpp"
#include "utils/inputUtils.hpp"

namespace Game {
HangmanGame::HangmanGame() {}

Hangman::Config HangmanGame::getConfigFromUser() {
  Hangman::Config config;

  std::cout << "Configure Hangman solver:\n";
  std::string patternStr = Utils::Input::promptString(
      "Enter word patterns (e.g., \"____ _A_ _____\")", "____");
  config.wordPatterns = Hangman::parsePatternString(patternStr);

  if (config.wordPatterns.empty()) {
    std::cout << "Using default pattern: ____\n";
    config.wordPatterns = {{"____"}};
  }

  config.autoDepth = Utils::Input::promptBool(
      "Use auto-depth calculation (Recommended)?", true);
  if (!config.autoDepth) {
    config.maxDepth = static_cast<uint8_t>(Utils::Input::promptInt(
        "Search depth for ENT calculation (0-2)", 1, 0, 2));
  }
  config.excludeUncommonWords = Utils::Input::promptBool(
      "Exclude uncommon words from suggestions?", true);
  config.maxGuesses =
      Utils::Input::promptInt("Maximum number of strikes allowed", 6, 1, 100);

  return config;
}

Hangman::Config
HangmanGame::getConfigFromArgs(const std::map<std::string, std::string> &args) {
  Hangman::Config config;

  // Check for combined "input" format: "pattern;strikes"
  auto inputIt = args.find("input");
  if (inputIt != args.end()) {
    std::string input = inputIt->second;
    size_t semicolonPos = input.find(';');
    if (semicolonPos != std::string::npos) {
      std::string patternStr = input.substr(0, semicolonPos);
      std::string strikesStr = input.substr(semicolonPos + 1);
      config.wordPatterns = Hangman::parsePatternString(patternStr);
      config.feedbackHistory = Hangman::parseStrikes(strikesStr);
    } else {
      // No semicolon - treat as pattern only
      config.wordPatterns = Hangman::parsePatternString(input);
    }
  } else {
    // Fallback to separate --pattern and --strikes args
    auto patternIt = args.find("pattern");
    if (patternIt != args.end()) {
      config.wordPatterns = Hangman::parsePatternString(patternIt->second);
    }
    config.feedbackHistory = getFeedbackFromArgs(args);
  }

  if (config.wordPatterns.empty()) {
    // Fallback to old min/max word length for backwards compatibility
    int minLen = Utils::Input::getArgValue(args, "min-word-length", 4);
    std::string defaultPattern(static_cast<size_t>(minLen), '_');
    config.wordPatterns = {{defaultPattern}};
  }

  config.maxDepth =
      static_cast<uint8_t>(Utils::Input::getArgValue(args, "max-depth", 0));
  config.autoDepth = Utils::Input::getArgValue(args, "auto-depth", false);
  config.excludeUncommonWords =
      Utils::Input::getArgValue(args, "exclude-uncommon-words", false);
  config.maxGuesses = Utils::Input::getArgValue(args, "max-guesses", 6);
  return config;
}

std::vector<Hangman::Feedback> HangmanGame::getFeedbackFromArgs(
    const std::map<std::string, std::string> &args) {
  std::vector<Hangman::Feedback> feedbackHistory;

  // Look for strikes argument (letters NOT in the word)
  auto it = args.find("strikes");
  if (it == args.end()) {
    return feedbackHistory;
  }

  // Parse strikes string - simple list of letters not in word (e.g., "etxzq")
  return Hangman::parseStrikes(it->second);
}

void HangmanGame::printResults(const Hangman::Result &result) {
  std::cout << "\n=== HANGMAN SOLVER RESULTS ===\n";

  if (result.totalPossiblePatterns == 0) {
    std::cout << "No possible words found with given constraints.\n";
    return;
  }

  std::cout << "Possible patterns (phrases) remaining: "
            << result.totalPossiblePatterns << "\n";
  std::cout << "Possible unique words: " << result.possibleWords.size() << "\n";
  std::cout << "Search depth used: " << result.searchDepth << "\n";

  if (!result.sortedGuesses.empty()) {
    std::cout << "=== Best letter guesses ===\n";

    // Header
    std::cout << std::setw(10) << "Rank";
    std::cout << std::setw(10) << "Letter";
    std::cout << std::setw(12) << "ENT Score";
    std::cout << std::setw(12) << "WNT Score";
    std::cout << std::setw(15) << "In Word %" << "\n";

    int totalWidth = 10 + 10 + 12 + 12 + 15;
    std::cout << std::string(static_cast<size_t>(std::max(0, totalWidth)), '-')
              << "\n";

    // Print top 10 letter guesses
    int count = std::min(10, static_cast<int>(result.sortedGuesses.size()));
    for (int i = 0; i < count; ++i) {
      const auto &guess = result.sortedGuesses[i];
      std::cout << std::setw(10) << (i + 1);
      std::cout << std::setw(10)
                << static_cast<char>(std::toupper(guess.letter));
      std::cout << std::setw(12) << std::fixed << std::setprecision(3)
                << guess.ent;
      std::cout << std::setw(12) << std::fixed << std::setprecision(3)
                << guess.wnt;
      std::cout << std::setw(15) << std::fixed << std::setprecision(1)
                << (guess.probability * 100.0) << "%\n";
    }

    std::cout << std::string(static_cast<size_t>(std::max(0, totalWidth)), '-')
              << "\n";
  }

  // Show some possible words
  if (!result.possibleWords.empty()) {
    std::cout << "\n=== Sample possible words ===\n";
    int wordCount = std::min(20, static_cast<int>(result.possibleWords.size()));
    for (int i = 0; i < wordCount; ++i) {
      std::cout << result.possibleWords[i].wordString;
      if (i < wordCount - 1)
        std::cout << ", ";
    }
    if (result.totalPossiblePatterns > 20) {
      std::cout << " ... and " << (result.totalPossiblePatterns - 20)
                << " more";
    }
    std::cout << "\n";
  }
}

void HangmanGame::saveResults(const Hangman::Result &result,
                              const std::string &outputFile) {
  std::filesystem::path outputPath(outputFile);
  if (!outputPath.parent_path().empty() &&
      !std::filesystem::exists(outputPath.parent_path())) {
    std::filesystem::create_directories(outputPath.parent_path());
  }

  std::ofstream out(outputFile);
  if (out.is_open()) {
    // Write letter guesses (no header, just data)
    for (const auto &guess : result.sortedGuesses) {
      out << guess.letter << " " << guess.ent << " " << guess.wnt << " "
          << guess.probability << "\n";
    }
    // Write possible words (no gap, no header)
    for (const auto &word : result.possibleWords) {
      out << word.wordString << "\n";
    }
    out.close();
  } else {
    std::cerr << "Could not write to file: " << outputFile << "\n";
  }

  std::cout << result.totalPossiblePatterns << "\n";
  std::cout << result.sortedGuesses.size() << "\n";
  std::cout << outputFile << "\n";
  std::cout << result.searchDepth << "\n";
  std::cout << result.possibleWords.size();
}

void HangmanGame::runCLI() {
  Hangman::Config config;

  std::cout << "\n=== HANGMAN SOLVER SETUP ===\n";
  config.autoDepth = Utils::Input::promptBool(
      "Use auto-depth calculation (Recommended)?", true);
  if (!config.autoDepth) {
    config.maxDepth = static_cast<uint8_t>(Utils::Input::promptInt(
        "Search depth for ENT calculation (0-2)", 1, 0, 2));
  }
  config.excludeUncommonWords = Utils::Input::promptBool(
      "Exclude uncommon words from suggestions?", true);
  config.maxGuesses =
      Utils::Input::promptInt("Maximum number of strikes allowed", 6, 1, 100);

  std::cout << "\n=== HANGMAN SOLVER ===\n";
  std::cout << "Format: PATTERN;STRIKES (e.g., '_A__ ___; xyz')\n";
  std::cout << "  - PATTERN: Use '_' for unknown letters, actual letters for "
               "revealed positions\n";
  std::cout << "  - STRIKES: Letters guessed that are NOT in the phrase\n";
  std::cout << "  - Separate multiple words with spaces\n";
  std::cout << "Commands: 'quit' to exit\n\n";

  while (true) {
    try {
      std::string input = Utils::Input::promptString(
          "Enter pattern;strikes (e.g., '_A__ ___;xyz')");
      std::string trimmed = Utils::trimToLower(input);

      if (trimmed == "quit" || trimmed == "q" || trimmed == "exit") {
        std::cout << "Exiting Hangman solver.\n";
        break;
      }

      if (trimmed.empty()) {
        continue;
      }

      // Parse pattern;strikes format
      std::string patternStr;
      std::string strikesStr;

      size_t semicolonPos = trimmed.find(';');
      if (semicolonPos != std::string::npos) {
        patternStr = trimmed.substr(0, semicolonPos);
        strikesStr = trimmed.substr(semicolonPos + 1);
      } else {
        // No semicolon - treat entire input as pattern with no strikes
        patternStr = trimmed;
      }

      // Parse pattern
      config.wordPatterns = Hangman::parsePatternString(patternStr);
      if (config.wordPatterns.empty()) {
        std::cout << "Invalid pattern. Use '_' for unknown letters.\n";
        continue;
      }

      // Parse strikes
      config.feedbackHistory = Hangman::parseStrikes(strikesStr);

      std::cout << "\nPattern: "
                << Hangman::patternsToString(config.wordPatterns) << "\n";
      if (!strikesStr.empty()) {
        std::cout << "Strikes: ";
        for (char c : strikesStr) {
          if (std::isalpha(static_cast<unsigned char>(c))) {
            std::cout << static_cast<char>(std::toupper(c)) << " ";
          }
        }
        std::cout << "\n";
      }

      // Run solver
      std::cout << "\nCalculating best next letter...\n";
      Hangman::Result result = Hangman::runHangmanSolver(config);

      if (result.totalPossiblePatterns == 0) {
        std::cout << "\nNo words match these constraints. Check your inputs.\n";
        continue;
      }

      if (result.totalPossiblePatterns == 1) {
        std::cout << "\n*** SOLVED! The answer is: "
                  << result.possibleWords[0].wordString << " ***\n";
        continue;
      }

      printResults(result);

    } catch (const Utils::Input::UserCancelledException &) {
      std::cout << "\nOperation cancelled.\n";
      break;
    }
  }
}

void HangmanGame::runHeadless(const Utils::Input::CommandArgs &cmdArgs) {
  try {
    const auto &args = cmdArgs.flags;
    Hangman::Config config = getConfigFromArgs(args);

    Hangman::Result result = Hangman::runHangmanSolver(config);

    std::string outputFile =
        Utils::Input::getArgValue(args, "o", std::string(""));
    if (outputFile.empty()) {
      outputFile = Utils::Input::getArgValue(
          args, "output", std::string("results/hangman.txt"));
    }

    saveResults(result, outputFile);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}
} // namespace Game
