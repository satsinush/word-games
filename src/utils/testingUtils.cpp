#include "testingUtils.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <random>

#include "../letterBoxed/letterBoxed.hpp"
#include "../mastermind/mastermind.hpp"
#include "../spellingBee/spellingBee.hpp"
#include "../wordle/wordle.hpp"
#include "inputUtils.hpp"

namespace Utils {
namespace Testing {

BenchmarkResult runRuntimeBenchmark(const std::string &gameMode,
                                    const std::vector<Utils::Word> &wordVec,
                                    const BenchmarkConfig &config) {
  BenchmarkResult result;
  result.gameMode = gameMode;
  result.iterations = config.iterations;

  if (config.verbose) {
    std::cout << "Running runtime benchmark for " << gameMode << " ("
              << config.iterations << " iterations)...\n";
  }

  int64_t startTime = Utils::Profiling::getTime();

  for (int i = 0; i < config.iterations; ++i) {
    if (gameMode == "wordle") {
      // Run a basic Wordle solve with no feedback (all possible words)
      Wordle::Config wordleConfig;
      wordleConfig.maxDepth = 0; // Fast configuration
      wordleConfig.excludeUncommonWords = false;

      std::vector<Wordle::Feedback> emptyFeedback;
      Wordle::runWordleSolver(wordVec, emptyFeedback, wordleConfig);

    } else if (gameMode == "mastermind") {
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

    } else if (gameMode == "spellingbee") {
      // Run a basic Spelling Bee solve with test letters
      SpellingBee::Config spellingBeeConfig;
      // Set up test letters: A B C D E F G (A is center letter)
      std::string testLetters = "nhmkace";
      for (int j = 0; j < 7; ++j) {
        spellingBeeConfig.allLetters[j] = testLetters[j];
        spellingBeeConfig
            .validLettersMap[static_cast<unsigned char>(testLetters[j])] = true;
      }

      std::vector<Utils::Word> solutions =
          SpellingBee::runSpellingBeeSolver(wordVec, spellingBeeConfig);

    } else if (gameMode == "letterboxed") {
      // Run a basic Letter Boxed solve with test letters
      LetterBoxed::Config letterBoxedConfig;
      letterBoxedConfig.maxDepth = 2;
      letterBoxedConfig.minWordLength = 3;
      letterBoxedConfig.minUniqueLetters = 3;

      // Set up test letters: 12 letters, 3 per side (4 sides)
      std::string testLetters = "uvjswitgebac";

      // Initialize charToIndexMap to -1 (invalid)
      for (int k = 0; k < 256; ++k) {
        letterBoxedConfig.charToIndexMap[k] = -1;
      }

      for (int j = 0; j < 12; ++j) {
        letterBoxedConfig.allLetters[j] = testLetters[j];
        letterBoxedConfig.letterToSideMapping[j] =
            j / 3; // 3 letters per side (4 sides)
        letterBoxedConfig.uniquePuzzleLetters.set(j); // Set bit j (0-11)
        letterBoxedConfig
            .charToIndexMap[static_cast<unsigned char>(testLetters[j])] =
            j; // Map char to index j (0-11)
      }

      std::vector<LetterBoxed::Solution> solutions =
          LetterBoxed::runLetterBoxedSolver(letterBoxedConfig, wordVec);

    } else {
      throw std::runtime_error("Unsupported game mode for runtime benchmark: " +
                               gameMode);
    }

    if (config.verbose && (i + 1) % std::max(1, config.iterations / 10) == 0) {
      std::cout << "Completed " << (i + 1) << "/" << config.iterations
                << " iterations\n";
    }
  }

  int64_t endTime = Utils::Profiling::getTime();

  result.totalTimeMs = (endTime - startTime) * Utils::Profiling::NANO_TO_SEC *
                       1000.0; // Convert nanoseconds to milliseconds
  result.averageTimeMs = result.totalTimeMs / config.iterations;

  return result;
}

BenchmarkResult runPerformanceBenchmark(const std::string &gameMode,
                                        const std::vector<Utils::Word> &wordVec,
                                        const BenchmarkConfig &config) {
  BenchmarkResult result;
  result.gameMode = gameMode;

  if (gameMode != "wordle") {
    throw std::runtime_error(
        "Performance benchmark is only available for Wordle");
  }

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

void printBenchmarkResults(const BenchmarkResult &result) {
  std::cout << "\n=== BENCHMARK RESULTS ===\n";
  std::cout << "Game Mode: " << result.gameMode << "\n";

  if (result.iterations > 0) {
    std::cout << "Iterations: " << result.iterations << "\n";
    std::cout << "Total Time: " << std::fixed << std::setprecision(2)
              << result.totalTimeMs << " ms\n";
    std::cout << "Average Time: " << std::fixed << std::setprecision(2)
              << result.averageTimeMs << " ms per iteration\n";
  }

  if (result.totalGames > 0) {
    std::cout << "Games Tested: " << result.totalGames << "\n";
    std::cout << "Average Guesses: " << std::fixed << std::setprecision(3)
              << result.averageGuesses << "\n";
    std::cout << "Min Guesses: " << result.minGuesses << "\n";
    std::cout << "Max Guesses: " << result.maxGuesses << "\n";

    if (result.totalTimeMs > 0) {
      std::cout << "Average Time per Game: " << std::fixed
                << std::setprecision(2)
                << result.totalTimeMs / result.totalGames << " ms\n";
    }
  }

  std::cout << "========================\n\n";
}

BenchmarkConfig
parseBenchmarkArgs(const std::map<std::string, std::string> &args) {
  BenchmarkConfig config;

  config.gameMode = Utils::Input::getArgValue(args, "mode", std::string(""));
  config.iterations = Utils::Input::getArgValue(args, "iterations", 1);
  // Check for both -v and --verbose
  config.verbose = Utils::Input::getArgValue(args, "v", false) ||
                   Utils::Input::getArgValue(args, "verbose", false);

  return config;
}

} // namespace Testing
} // namespace Utils