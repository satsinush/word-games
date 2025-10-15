#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

#include "../utils/inputUtils.hpp"
#include "SpellingBeeGame.hpp"

namespace Game {
SpellingBeeGame::SpellingBeeGame(const std::vector<Utils::Word> &words)
    : wordVec(words) {}

void SpellingBeeGame::drawPuzzle(const std::array<char, 7> &letters) {
  auto up = [](char c) {
    return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  };
  std::cout << std::endl;
  std::cout << "      " << up(letters[1]) << std::endl;
  std::cout << "   " << up(letters[6]) << "     " << up(letters[2])
            << std::endl;
  std::cout << "      " << up(letters[0]) << std::endl;
  std::cout << "   " << up(letters[5]) << "     " << up(letters[3])
            << std::endl;
  std::cout << "      " << up(letters[4]) << std::endl << std::endl;
}

SpellingBee::Config SpellingBeeGame::getConfigFromUser() {
  SpellingBee::Config config;

  // Get puzzle letters (7 unique letters)
  std::string letters = Utils::Input::promptLetters(
      "Enter the 7 puzzle letters (ex. a bcdefg):", 7, false);

  for (size_t i = 0; i < 7; ++i) {
    config.allLetters[i] = letters[i];
  }

  // Set up valid letters map
  for (char c : config.allLetters) {
    config.validLettersMap[static_cast<unsigned char>(c)] = true;
  }

  drawPuzzle(config.allLetters);

  return config;
}

SpellingBee::Config SpellingBeeGame::getConfigFromArgs(
    const std::map<std::string, std::string> &args) {
  SpellingBee::Config config;

  // Parse letters
  std::string letters =
      Utils::Input::getArgValue(args, "letters", std::string(""));
  if (letters.empty()) {
    throw std::invalid_argument("Missing required argument: letters");
  }

  letters.erase(std::remove_if(letters.begin(), letters.end(), ::isspace),
                letters.end());
  std::transform(letters.begin(), letters.end(), letters.begin(), ::tolower);

  if (letters.size() != 7) {
    throw std::invalid_argument(
        "Must provide exactly 7 letters for Spelling Bee.");
  }

  // Check for duplicates and validate letters
  std::set<char> seen;
  for (size_t i = 0; i < 7; ++i) {
    char c = letters[i];
    if (!isalpha(static_cast<unsigned char>(c)))
      throw std::invalid_argument("All characters must be letters.");
    if (seen.count(c))
      throw std::invalid_argument(
          "Duplicate letters not allowed in Spelling Bee.");
    seen.insert(c);
    config.allLetters[i] = c;
  }

  // Set up valid letters map
  for (char c : config.allLetters) {
    config.validLettersMap[static_cast<unsigned char>(c)] = true;
  }

  return config;
}

void SpellingBeeGame::printSolutions(
    const std::vector<Utils::Word> &solutions) {
  // Print top 100 solutions
  int toPrint = std::min(100, static_cast<int>(solutions.size()));

  int lastUniqueLetters = 0;
  int count = 0;

  for (auto it = solutions.rbegin(); it != solutions.rend() && count < toPrint;
       ++it, ++count) {
    if (lastUniqueLetters == 0 || (it->uniqueLetters != lastUniqueLetters)) {
      if (lastUniqueLetters != 0) {
        std::cout << "\n";
      }
      std::cout << "=== " << it->uniqueLetters << " unique letters ===\n";
    }
    std::cout << it->wordString << "\n";
    lastUniqueLetters = it->uniqueLetters;
  }

  std::cout << "\n" << solutions.size() << " valid word(s) found.";
  if (toPrint < static_cast<int>(solutions.size())) {
    std::cout << " (Showing top " << toPrint << ")";
  }
  std::cout << "\n";
}

void SpellingBeeGame::runCLI() {
  while (true) {
    try {
      SpellingBee::Config config = getConfigFromUser();

      std::cout << "Running solver...\n";
      std::vector<Utils::Word> solutions =
          SpellingBee::runSpellingBeeSolver(wordVec, config);

      printSolutions(solutions);

      while (true) {
        try {
          std::cout << "\nCommands: 's' (solve again)\n";
          std::cout << "Enter command: ";
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

          if (input == "s" || input == "solve")
            break;
        } catch (const Utils::Input::UserCancelledException &) {
          // User pressed EOF at result menu, return to game menu
          std::cout << "Returning to game menu...\n";
          return;
        }
      }
    } catch (const Utils::Input::UserCancelledException &) {
      // User pressed EOF during config, return to game menu
      std::cout << "Returning to game menu...\n";
      return;
    }
  }
}

void SpellingBeeGame::runHeadless(const Utils::Input::CommandArgs &cmdArgs) {
  try {
    const auto &args = cmdArgs.flags;
    SpellingBee::Config config = getConfigFromArgs(args);

    std::vector<Utils::Word> solutions =
        SpellingBee::runSpellingBeeSolver(wordVec, config);

    // Output to file if specified
    std::string outputFile =
        Utils::Input::getArgValue(args, "o", std::string(""));
    if (outputFile.empty()) {
      outputFile = Utils::Input::getArgValue(args, "output",
                                             std::string("results/temp.txt"));
    }

    // Ensure directory exists
    std::filesystem::path filePath(outputFile);
    if (!filePath.parent_path().empty() &&
        !std::filesystem::exists(filePath.parent_path())) {
      std::filesystem::create_directories(filePath.parent_path());
    }

    std::ofstream out(outputFile);
    if (out.is_open()) {
      for (const auto &word : solutions) {
        out << word.wordString << "\n";
      }
      out.close();
    } else {
      std::cerr << "Could not write to file: " << outputFile << "\n";
    }

    std::cout << solutions.size() << "\n";
    std::cout << outputFile;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}

void SpellingBeeGame::runGUI() {
  std::cout << "GUI mode not yet implemented for Spelling Bee.\n";
}

Utils::Testing::BenchmarkResult
SpellingBeeGame::runBenchmark(const Utils::Testing::BenchmarkConfig &config) {
  Utils::Testing::BenchmarkResult result;
  result.gameMode = "spellingbee";

  if (config.type == Utils::Testing::BenchmarkType::PERFORMANCE) {
    std::cout << "Performance benchmark is not available for Spelling Bee.\n";
    std::cout << "Running runtime benchmark instead.\n";
  }

  // Runtime benchmark
  result.iterations = config.iterations;

  if (config.verbose) {
    std::cout << "Running runtime benchmark for Spelling Bee ("
              << config.iterations << " iterations)...\n";
  }

  int64_t startTime = Utils::Profiling::getTime();

  for (int i = 0; i < config.iterations; ++i) {
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