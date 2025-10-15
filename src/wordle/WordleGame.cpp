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
      "Search depth for ENT calculation (0-2)", 1, 0, 2);
  config.excludeUncommonWords = Utils::Input::promptBool(
      "Exclude uncommon words from suggestions?", true);

  return config;
}

Wordle::Config
WordleGame::getConfigFromArgs(const std::map<std::string, std::string> &args) {
  Wordle::Config config;
  config.maxDepth = Utils::Input::getArgValue(args, "max-depth", 0);
  config.excludeUncommonWords =
      Utils::Input::getArgValue(args, "exclude-uncommon-words", false);
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
    std::cout << "=== Best guesses ===\n";

    // Header
    std::cout << std::setw(10) << "Rank";
    std::cout << std::setw(12) << "Word";
    std::cout << std::setw(12) << "Word Score";
    std::cout << std::setw(12) << "ENT Score";
    std::cout << std::setw(15) << "Probability" << "\n";

    int totalWidth = 10 + 12 + 12 + 12 + 15;
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
        out << guess.word.wordString << "\n";
      }
    }
    for (const auto &guess : result.sortedGuesses) {
      out << guess.word.wordString << "," << guess.ent << ","
          << guess.probability << "\n";
    }
    out.close();
  } else {
    std::cerr << "Could not write to file: " << outputFile << "\n";
  }

  std::cout << result.totalPossibleWords << "\n";
  std::cout << result.sortedGuesses.size() << "\n";
  std::cout << outputFile;
}

void WordleGame::runCLI() {
  std::vector<Wordle::Feedback> feedbackHistory;

  while (true) {
    try {
      std::cout << "\n=== WORDLE SOLVER ===\n";
      std::cout << "Commands: 's' (solve), 'c' (clear)\n";
      std::cout << "Format: WORD 01201 (0=grey, 1=yellow, 2=green)\n\n";

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
        feedbackHistory.clear();
        std::cout << "Feedback history cleared.\n";
        continue;
      }

      if (input == "s" || input == "solve") {
        try {
          Wordle::Config config = getConfigFromUser();

          std::cout << "Calculating best guesses...\n";
          Wordle::Result result =
              Wordle::runWordleSolver(wordVec, feedbackHistory, config);

          printResults(result);
        } catch (const Utils::Input::UserCancelledException &) {
          std::cout << "Solve cancelled.\n";
        }
        continue;
      }

      // Try to parse as feedback
      try {
        Wordle::Feedback fb = Wordle::parseFeedback(input);
        feedbackHistory.push_back(fb);
        std::cout << "Added feedback for " << fb.word << "\n";
      } catch (const std::exception &e) {
        std::cout << "Error: " << e.what() << "\n";
        std::cout << "Use format: WORD 01201 or commands: s, c\n";
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
    std::vector<Wordle::Feedback> feedbackHistory = getFeedbackFromArgs(args);

    Wordle::Result result =
        Wordle::runWordleSolver(wordVec, feedbackHistory, config);

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

void WordleGame::runGUI() {
  std::cout << "GUI mode not yet implemented for Wordle.\n";
}

Utils::Testing::BenchmarkResult
WordleGame::runBenchmark(const Utils::Testing::BenchmarkConfig &config) {
  Utils::Testing::BenchmarkResult result;
  result.gameMode = "wordle";

  if (config.type == Utils::Testing::BenchmarkType::RUNTIME) {
    // Runtime benchmark - measures execution time
    result.iterations = config.iterations;

    if (config.verbose) {
      std::cout << "Running runtime benchmark for Wordle (" << config.iterations
                << " iterations)...\n";
    }

    int64_t startTime = Utils::Profiling::getTime();

    for (int i = 0; i < config.iterations; ++i) {
      // Run a basic Wordle solve with no feedback (all possible words)
      Wordle::Config wordleConfig;
      wordleConfig.maxDepth = 0; // Fast configuration
      wordleConfig.excludeUncommonWords = false;

      std::vector<Wordle::Feedback> emptyFeedback;
      Wordle::runWordleSolver(wordVec, emptyFeedback, wordleConfig);

      if (config.verbose &&
          (i + 1) % std::max(1, config.iterations / 10) == 0) {
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

  // Performance benchmark - measures solving performance
  // Filter to get 5-letter words and sort by score (highest first)
  std::vector<Utils::Word> fiveLetterWords;
  for (const auto &word : wordVec) {
    if (word.wordString.length() == 5) {
      fiveLetterWords.push_back(word);
    }
  }

  // Sort by score (descending) to get the top words
  std::sort(fiveLetterWords.begin(), fiveLetterWords.end(),
            [](const Utils::Word &a, const Utils::Word &b) {
              return a.score > b.score;
            });

  // Take top 1000 words (or all if less than 1000)
  int testWords = std::min(1000, static_cast<int>(fiveLetterWords.size()));
  result.totalGames = testWords;

  if (config.verbose) {
    std::cout << "Running Wordle performance benchmark on top " << testWords
              << " words...\n";
  }

  int64_t startTime = Utils::Profiling::getTime();

  int totalGuesses = 0;
  int minGuesses = INT_MAX;
  int maxGuesses = 0;

  Wordle::Config solverConfig;
  solverConfig.maxDepth = 1; // Reasonable performance vs accuracy tradeoff
  solverConfig.excludeUncommonWords = true;

  Utils::Profiling::g_process.start();
  for (int i = 0; i < testWords; ++i) {
    Utils::Profiling::g_process.update(static_cast<double>(i) / testWords);
    const Utils::Word &targetWord = fiveLetterWords[i];
    std::vector<Wordle::Feedback> feedbackHistory;

    int guesses = 0;
    const int maxAttempts = 6; // Standard Wordle limit

    // Simulate solving the target word
    while (guesses < maxAttempts) {
      std::string guessWord;

      if (guesses == 0) {
        // Always start with "TARES" as the first guess
        guessWord = "tares";
      } else {
        // Get solver suggestions for subsequent guesses
        Wordle::Result solverResult = Wordle::runWordleSolver(
            fiveLetterWords, feedbackHistory, solverConfig);

        if (solverResult.sortedGuesses.empty()) {
          break; // No more guesses possible
        }

        // Take the best guess (first in sorted list)
        guessWord = solverResult.sortedGuesses[0].word.wordString;
      }

      guesses++;

      // Check if we found the target
      if (guessWord == targetWord.wordString) {
        break; // Found it!
      }

      // Generate feedback for this guess
      Wordle::Feedback feedback =
          Wordle::generateFeedback(targetWord, guessWord);
      feedbackHistory.push_back(feedback);
    }

    totalGuesses += guesses;
    minGuesses = std::min(minGuesses, guesses);
    maxGuesses = std::max(maxGuesses, guesses);

    if (config.verbose && (i + 1) % 100 == 0) {
      std::cout << "Solved " << (i + 1) << "/" << testWords
                << " words (avg: " << std::fixed << std::setprecision(2)
                << static_cast<double>(totalGuesses) / (i + 1) << " guesses)\n";
    }
  }
  Utils::Profiling::g_process.stop();

  int64_t endTime = Utils::Profiling::getTime();

  result.totalTimeMs = (endTime - startTime) * Utils::Profiling::NANO_TO_SEC *
                       1000.0; // Convert nanoseconds to milliseconds
  result.averageTimeMs = result.totalTimeMs / testWords;
  result.averageGuesses = static_cast<double>(totalGuesses) / testWords;
  result.minGuesses = (minGuesses == INT_MAX) ? 0 : minGuesses;
  result.maxGuesses = maxGuesses;

  return result;
}
} // namespace Game