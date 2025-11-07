#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

#include "spellingBee/SpellingBeeGame.hpp"
#include "utils/inputUtils.hpp"

namespace Game {
SpellingBeeGame::SpellingBeeGame() {}

void SpellingBeeGame::drawPuzzle(const std::vector<char> &letters) {
  auto up = [](char c) {
    return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  };
  std::cout << std::endl;
  std::cout << "Letters: ";
  for (size_t i = 0; i < letters.size(); ++i) {
    if (i == 0) {
      std::cout << "[" << up(letters[i]) << "]"; // Brackets for first letter
    } else {
      std::cout << " " << up(letters[i]);
    }
  }
  std::cout << std::endl;
  std::cout << "(First letter in brackets must be included)" << std::endl
            << std::endl;
}

SpellingBee::Config SpellingBeeGame::getConfigFromUser() {
  SpellingBee::Config config;

  // Get puzzle letters (any number of letters, minimum 3, duplicates allowed)
  std::string letters = Utils::Input::promptLetters(
      "Enter puzzle letters (minimum 3, duplicates allowed, ex. a bcdefg):", 3,
      true);

  // Store letters in vector
  config.allLetters.clear();
  for (char c : letters) {
    config.allLetters.push_back(c);
  }

  // Set up valid letters map
  for (char c : config.allLetters) {
    config.validLettersMap[static_cast<unsigned char>(c)] = true;
  }

  config.excludeUncommonWords =
      Utils::Input::promptBool("Exclude uncommon words?", false);

  config.mustIncludeFirstLetter =
      Utils::Input::promptBool("Must include first letter?", true);

  config.reuseLetters =
      Utils::Input::promptBool("Allow reuse of letters?", true);

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

  if (letters.size() < 3) {
    throw std::invalid_argument(
        "Must provide at least 3 letters for Spelling Bee.");
  }

  // Validate letters (duplicates now allowed)
  config.allLetters.clear();
  for (char c : letters) {
    if (!isalpha(static_cast<unsigned char>(c)))
      throw std::invalid_argument("All characters must be letters.");
    config.allLetters.push_back(c);
  }

  // Set up valid letters map
  for (char c : config.allLetters) {
    config.validLettersMap[static_cast<unsigned char>(c)] = true;
  }

  config.excludeUncommonWords =
      Utils::Input::getArgValue(args, "exclude-uncommon-words", false);

  config.mustIncludeFirstLetter =
      Utils::Input::getArgValue(args, "must-include-first-letter", true);

  config.reuseLetters = Utils::Input::getArgValue(args, "reuse-letters", true);

  return config;
}

void SpellingBeeGame::printSolutions(
    const std::vector<Utils::Word> &solutions) {
  // Print top 100 solutions in reverse order (i.e. show best-ranked
  // solutions but print from lower-ranked of the top group up to the best).
  int total = static_cast<int>(solutions.size());
  int toPrint = std::min(100, total);

  int lastUniqueLetters = 0;

  // If there are no solutions, just print summary
  if (toPrint == 0) {
    std::cout << "\n" << solutions.size() << " valid word(s) found.";
    std::cout << "\n";
    return;
  }

  // Print indices [toPrint-1 .. 0]
  for (int idx = toPrint - 1; idx >= 0; --idx) {
    const auto &w = solutions[idx];
    if (lastUniqueLetters == 0 || (w.uniqueLetters != lastUniqueLetters)) {
      if (lastUniqueLetters != 0) {
        std::cout << "\n";
      }
      std::cout << "=== " << w.uniqueLetters << " unique letters ===\n";
    }
    std::cout << w.wordString << "\n";
    lastUniqueLetters = w.uniqueLetters;
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
      SpellingBee::Result result = SpellingBee::runSpellingBeeSolver(config);

      printSolutions(result.words);

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

    SpellingBee::Result result = SpellingBee::runSpellingBeeSolver(config);

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
      for (const auto &word : result.words) {
        out << word.wordString << "\n";
      }
      out.close();
    } else {
      std::cerr << "Could not write to file: " << outputFile << "\n";
    }

    std::cout << result.words.size() << "\n";
    std::cout << outputFile;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}
} // namespace Game