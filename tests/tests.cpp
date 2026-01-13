#include <algorithm>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <vector>

#include "dungleon/dungleon.hpp"
#include "hangman/hangman.hpp"
#include "letterBoxed/letterBoxed.hpp"
#include "mastermind/mastermind.hpp"
#include "spellingBee/spellingBee.hpp"
#include "utils/inputUtils.hpp"
#include "utils/utils.hpp"
#include "wordle/wordle.hpp"

// Forward declaration
static Utils::Word makeWord(const std::string &s);

// Helper function to load words for testing
std::vector<Utils::Word> loadTestWords() {
  // Load from test CSV without binary cache, load all words
  std::vector<Utils::Word> words =
      Utils::loadWords("test_word_scores.csv", false, 0);
  EXPECT_FALSE(words.empty()) << "Failed to load test word list";
  EXPECT_EQ(words.size(), 18)
      << "Test word list should contain exactly 18 words";
  return words;
}

// =============================================================================
// WORDLE TESTS
// =============================================================================

TEST(WordleTest, BasicFeedbackParsing) {
  Wordle::Feedback fb = Wordle::parseFeedback("STEAL 20100");
  EXPECT_EQ(fb.word, "steal");
  EXPECT_EQ(fb.getColor(0), 2); // S - green
  EXPECT_EQ(fb.getColor(1), 0); // T - grey
  EXPECT_EQ(fb.getColor(2), 1); // E - yellow
  EXPECT_EQ(fb.getColor(3), 0); // A - grey
  EXPECT_EQ(fb.getColor(4), 0); // L - grey
}

TEST(WordleTest, SolverWithGuesses) {
  // Load word list
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  // Parse command: wordle --max-depth 0 --guesses "STEAL 20100;CRANE 01002"
  std::map<std::string, std::string> args;
  args["max-depth"] = "0";
  args["guesses"] = "STEAL 20100;CRANE 01002";

  // Build config
  Wordle::Config config;
  config.maxDepth = std::stoi(args["max-depth"]);

  // Parse feedback history
  std::string guessesStr = args["guesses"];
  size_t pos = 0;
  while ((pos = guessesStr.find(';')) != std::string::npos) {
    std::string token = guessesStr.substr(0, pos);
    config.feedbackHistory.push_back(Wordle::parseFeedback(token));
    guessesStr.erase(0, pos + 1);
  }
  if (!guessesStr.empty()) {
    config.feedbackHistory.push_back(Wordle::parseFeedback(guessesStr));
  }

  EXPECT_EQ(config.feedbackHistory.size(), 2);

  // Run solver
  Wordle::Result result = Wordle::runWordleSolver(config);

  // Verify results
  EXPECT_GT(result.sortedGuesses.size(), 0) << "Should have at least one guess";
  EXPECT_EQ(result.totalPossibleWords, 2)
      << "Should have exactly 2 possible words remaining";
}

TEST(WordleTest, SolverNoGuesses) {
  // Test solver with no prior guesses
  Wordle::Config config;
  config.maxDepth = 0;

  Wordle::Result result = Wordle::runWordleSolver(config);

  EXPECT_GT(result.sortedGuesses.size(), 0) << "Should return suggestions";
  EXPECT_GT(result.totalPossibleWords, 0) << "Should have possible words";
}

TEST(WordleTest, GenerateFeedbackConsistency) {
  Utils::Word target = makeWord("tares");
  std::string guess = "steal";

  Wordle::Feedback fb1 = Wordle::generateFeedback(target, guess);
  Wordle::Feedback fb2 = Wordle::generateFeedback(target, guess);

  EXPECT_EQ(fb1.word, fb2.word);
  for (size_t i = 0; i < 5; ++i) {
    EXPECT_EQ(fb1.getColor(i), fb2.getColor(i));
  }
}

TEST(WordleTest, MatchesFeedbackCorrect) {
  Utils::Word target = makeWord("tares");
  std::string guess = "steal";

  Wordle::Feedback fb = Wordle::generateFeedback(target, guess);
  EXPECT_TRUE(Wordle::matchesFeedback(target, fb));
}

// =============================================================================
// MASTERMIND TESTS
// =============================================================================

TEST(MastermindTest, PatternGeneration) {
  Mastermind::Config config;
  config.numPegs = 4;
  config.colorChars = "012345";
  config.allowDuplicates = true;

  std::vector<Mastermind::Pattern> patterns =
      Mastermind::generateAllPatterns(config);

  // With 4 pegs and 6 colors, allowing duplicates: 6^4 = 1296
  EXPECT_EQ(patterns.size(), 1296);
}

TEST(MastermindTest, SolverWithGuesses) {
  // Parse command: mastermind --guesses "1122 1 2;2131 2 1"
  //                --num-pegs 4 --colors 0123456789 --allow-duplicates true
  //                --max-depth 0
  Mastermind::Config config;
  config.numPegs = 4;
  config.colorChars = "012345"; // 6 colors
  config.allowDuplicates = true;
  config.maxDepth = 0;

  // Generate all patterns
  std::vector<Mastermind::Pattern> allPatterns =
      Mastermind::generateAllPatterns(config);
  EXPECT_EQ(allPatterns.size(), 1296);

  // First guess: "1122 1 2"
  config.feedbackHistory.push_back(
      Mastermind::parseFeedback("1122 1 2", config));

  // Second guess: "2131 2 1"
  config.feedbackHistory.push_back(
      Mastermind::parseFeedback("2131 2 1", config));

  // Run solver
  Mastermind::Result result = Mastermind::runMastermindSolver(config);

  // Verify results
  EXPECT_GT(result.sortedGuesses.size(), 0) << "Should have at least one guess";
  EXPECT_GT(result.totalPossiblePatterns, 0)
      << "Should have possible patterns remaining";
}

TEST(MastermindTest, SolverNoGuesses) {
  // Test solver with no prior guesses
  Mastermind::Config config;
  config.numPegs = 4;
  config.colorChars = "0123";
  config.allowDuplicates = true;
  config.maxDepth = 0;

  Mastermind::Result result = Mastermind::runMastermindSolver(config);

  EXPECT_GT(result.sortedGuesses.size(), 0) << "Should return suggestions";
  EXPECT_GT(result.totalPossiblePatterns, 0) << "Should have possible patterns";
}

TEST(MastermindTest, GenerateFeedbackSymmetry) {
  Mastermind::Config config;
  config.numPegs = 4;
  config.colorChars = "012345";
  config.allowDuplicates = true;

  Mastermind::Pattern p1 = Mastermind::parseFeedback("1234 0 0", config).guess;
  Mastermind::Pattern p2 = Mastermind::parseFeedback("5432 0 0", config).guess;

  Mastermind::Feedback fb1 = Mastermind::generateFeedback(p1, p2);
  Mastermind::Feedback fb2 = Mastermind::generateFeedback(p2, p1);

  // Feedback should be symmetric for same patterns
  EXPECT_EQ(fb1.correctPosition, fb2.correctPosition);
  EXPECT_EQ(fb1.correctColor, fb2.correctColor);
}

TEST(MastermindTest, PatternToStringConsistency) {
  Mastermind::Config config;
  config.numPegs = 4;
  config.colorChars = "012345";
  config.allowDuplicates = true;

  Mastermind::Pattern p = Mastermind::parseFeedback("1234 0 0", config).guess;
  std::string str = p.toString(config);

  EXPECT_EQ(str, "1234");
}

// =============================================================================
// SPELLING BEE TESTS
// =============================================================================

TEST(SpellingBeeTest, ConfigValidation) {
  SpellingBee::Config config;
  config.allLetters = {{'e', 's', 'r', 't', 'a', 'n', 'o'}};

  // Set up valid letters map
  for (char c : config.allLetters) {
    config.validLettersMap[static_cast<unsigned char>(c)] = true;
  }

  EXPECT_EQ(config.allLetters.size(), 7);
  EXPECT_TRUE(config.validLettersMap[static_cast<unsigned char>('e')]);
  EXPECT_TRUE(config.validLettersMap[static_cast<unsigned char>('a')]);
}

TEST(SpellingBeeTest, SolverWithLetters) {
  // Load word list
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  // Parse command: spellingbee --letters esrtano
  SpellingBee::Config config;
  std::string lettersStr = "esrtano";

  // Parse letters into config
  for (size_t i = 0; i < lettersStr.length() && i < 7; ++i) {
    config.allLetters.push_back(lettersStr[i]);
  }

  // Set up valid letters map
  for (char c : config.allLetters) {
    config.validLettersMap[static_cast<unsigned char>(c)] = true;
  }

  // Run solver
  SpellingBee::Result result = SpellingBee::runSpellingBeeSolver(config);

  // Verify results
  EXPECT_EQ(result.words.size(), 4) << "Should find exactly 4 solutions";

  // Verify all solutions only use allowed letters
  for (const auto &word : result.words) {
    for (char c : word.wordString) {
      EXPECT_TRUE(config.validLettersMap[static_cast<unsigned char>(c)])
          << "Word '" << word.wordString << "' contains invalid letter '" << c
          << "'";
    }
  }
}

TEST(SpellingBeeTest, SolverExcludeUncommon) {
  // Load word list
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  SpellingBee::Config config;
  std::string lettersStr = "esrtano";

  for (size_t i = 0; i < lettersStr.length() && i < 7; ++i) {
    config.allLetters.push_back(lettersStr[i]);
  }

  for (char c : config.allLetters) {
    config.validLettersMap[static_cast<unsigned char>(c)] = true;
  }

  config.excludeUncommonWords = true;

  SpellingBee::Result result = SpellingBee::runSpellingBeeSolver(config);

  // Verify all solutions are marked as common (is_scrabble)
  for (const auto &word : result.words) {
    EXPECT_TRUE(word.is_scrabble)
        << "Word '" << word.wordString << "' should be common";
  }
}

TEST(SpellingBeeTest, CenterLetterRequired) {
  // Load word list
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  SpellingBee::Config config;
  std::string lettersStr = "esrtano";

  for (size_t i = 0; i < lettersStr.length() && i < 7; ++i) {
    config.allLetters.push_back(lettersStr[i]);
  }

  for (char c : config.allLetters) {
    config.validLettersMap[static_cast<unsigned char>(c)] = true;
  }

  SpellingBee::Result result = SpellingBee::runSpellingBeeSolver(config);

  // All solutions must contain the center letter (first letter)
  char centerLetter = config.allLetters[0];
  for (const auto &word : result.words) {
    bool hasCenter = false;
    for (char c : word.wordString) {
      if (c == centerLetter) {
        hasCenter = true;
        break;
      }
    }
    EXPECT_TRUE(hasCenter) << "Word '" << word.wordString
                           << "' must contain center letter '" << centerLetter
                           << "'";
  }
}

TEST(SpellingBeeTest, MinimumWordLength) {
  // All solutions must be at least 4 letters
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  SpellingBee::Config config;
  std::string lettersStr = "esrtano";

  for (size_t i = 0; i < lettersStr.length() && i < 7; ++i) {
    config.allLetters.push_back(lettersStr[i]);
  }

  for (char c : config.allLetters) {
    config.validLettersMap[static_cast<unsigned char>(c)] = true;
  }

  SpellingBee::Result result = SpellingBee::runSpellingBeeSolver(config);

  for (const auto &word : result.words) {
    EXPECT_GE(word.wordString.length(), 4)
        << "Word '" << word.wordString << "' must be at least 4 letters";
  }
}

// =============================================================================
// LETTER BOXED TESTS
// =============================================================================

TEST(LetterBoxedTest, ConfigPreset) {
  // Parse command: letterboxed --preset 1 --letters esrtanopdilc
  LetterBoxed::Config config;
  std::string lettersStr = "esrtanopdilc";

  // Preset 1 configuration (from getConfigFromArgs)
  config.maxDepth = 2;
  config.minWordLength = 3;
  config.minUniqueLetters = 2;
  config.pruneRedundantPaths = true;
  config.pruneDominatedClasses = false;

  // Parse letters (12 letters, 3 per side)
  ASSERT_EQ(lettersStr.length(), 12) << "Letter boxed needs exactly 12 letters";

  for (size_t i = 0; i < 12; ++i) {
    config.allLetters[i] = lettersStr[i];
    config.uniquePuzzleLetters.set(i);
  }

  // Set side mapping (3 letters per side)
  for (int i = 0; i < 3; ++i)
    config.letterToSideMapping[i] = 0;
  for (int i = 3; i < 6; ++i)
    config.letterToSideMapping[i] = 1;
  for (int i = 6; i < 9; ++i)
    config.letterToSideMapping[i] = 2;
  for (int i = 9; i < 12; ++i)
    config.letterToSideMapping[i] = 3;

  // Set up char to index map
  config.charToIndexMap.fill(-1);
  for (int i = 0; i < 12; ++i) {
    config.charToIndexMap[static_cast<unsigned char>(config.allLetters[i])] = i;
  }

  EXPECT_EQ(config.maxDepth, 2);
  EXPECT_EQ(config.minWordLength, 3);
  EXPECT_EQ(config.minUniqueLetters, 2);
  EXPECT_TRUE(config.pruneRedundantPaths);
  EXPECT_FALSE(config.pruneDominatedClasses);
}

TEST(LetterBoxedTest, SolverWithLetters) {
  // Load word list
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  // Parse command: letterboxed --preset 1 --letters esrtanopdilc
  LetterBoxed::Config config;
  std::string lettersStr = "esrtanopdilc";

  // Preset 1 configuration
  config.maxDepth = 2;
  config.minWordLength = 3;
  config.minUniqueLetters = 2;
  config.pruneRedundantPaths = true;
  config.pruneDominatedClasses = false;

  // Parse letters
  for (size_t i = 0; i < 12; ++i) {
    config.allLetters[i] = lettersStr[i];
    config.uniquePuzzleLetters.set(i);
  }

  // Set side mapping
  for (int i = 0; i < 3; ++i)
    config.letterToSideMapping[i] = 0;
  for (int i = 3; i < 6; ++i)
    config.letterToSideMapping[i] = 1;
  for (int i = 6; i < 9; ++i)
    config.letterToSideMapping[i] = 2;
  for (int i = 9; i < 12; ++i)
    config.letterToSideMapping[i] = 3;

  // Set up char to index map
  config.charToIndexMap.fill(-1);
  for (int i = 0; i < 12; ++i) {
    config.charToIndexMap[static_cast<unsigned char>(config.allLetters[i])] = i;
  }

  // Run solver
  LetterBoxed::Result result = LetterBoxed::runLetterBoxedSolver(config);

  // Verify results
  EXPECT_EQ(result.solutions.size(), 4) << "Should find exactly 4 solutions";

  // Verify solutions are sorted correctly
  for (size_t i = 1; i < result.solutions.size(); ++i) {
    EXPECT_TRUE(result.solutions[i - 1] < result.solutions[i] ||
                !(result.solutions[i] < result.solutions[i - 1]))
        << "Solutions should be sorted";
  }
}

TEST(LetterBoxedTest, SideConstraints) {
  // Verify that consecutive letters can't be from the same side
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  LetterBoxed::Config config;
  std::string lettersStr = "esrtanopdilc";

  config.maxDepth = 2;
  config.minWordLength = 3;
  config.minUniqueLetters = 2;
  config.pruneRedundantPaths = true;
  config.pruneDominatedClasses = false;

  for (size_t i = 0; i < 12; ++i) {
    config.allLetters[i] = lettersStr[i];
    config.uniquePuzzleLetters.set(i);
  }

  for (int i = 0; i < 3; ++i)
    config.letterToSideMapping[i] = 0;
  for (int i = 3; i < 6; ++i)
    config.letterToSideMapping[i] = 1;
  for (int i = 6; i < 9; ++i)
    config.letterToSideMapping[i] = 2;
  for (int i = 9; i < 12; ++i)
    config.letterToSideMapping[i] = 3;

  config.charToIndexMap.fill(-1);
  for (int i = 0; i < 12; ++i) {
    config.charToIndexMap[static_cast<unsigned char>(config.allLetters[i])] = i;
  }

  LetterBoxed::Result result = LetterBoxed::runLetterBoxedSolver(config);

  // Verify each solution respects side constraints
  for (const auto &solution : result.solutions) {
    // Parse words from solution text
    std::string text = solution.text;
    size_t pos = 0;
    std::vector<std::string> solutionWords;
    while ((pos = text.find(" + ")) != std::string::npos) {
      solutionWords.push_back(text.substr(0, pos));
      text.erase(0, pos + 3);
    }
    if (!text.empty()) {
      solutionWords.push_back(text);
    }

    for (const auto &word : solutionWords) {
      for (size_t i = 1; i < word.length(); ++i) {
        int idx1 =
            config.charToIndexMap[static_cast<unsigned char>(word[i - 1])];
        int idx2 = config.charToIndexMap[static_cast<unsigned char>(word[i])];
        if (idx1 >= 0 && idx2 >= 0) {
          EXPECT_NE(config.letterToSideMapping[idx1],
                    config.letterToSideMapping[idx2])
              << "Consecutive letters '" << word[i - 1] << "' and '" << word[i]
              << "' in word '" << word << "' are on the same side";
        }
      }
    }
  }
}

TEST(LetterBoxedTest, AllLettersUsed) {
  // All solutions must use all 12 letters
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  LetterBoxed::Config config;
  std::string lettersStr = "esrtanopdilc";

  config.maxDepth = 2;
  config.minWordLength = 3;
  config.minUniqueLetters = 2;
  config.pruneRedundantPaths = true;
  config.pruneDominatedClasses = false;

  for (size_t i = 0; i < 12; ++i) {
    config.allLetters[i] = lettersStr[i];
    config.uniquePuzzleLetters.set(i);
  }

  for (int i = 0; i < 3; ++i)
    config.letterToSideMapping[i] = 0;
  for (int i = 3; i < 6; ++i)
    config.letterToSideMapping[i] = 1;
  for (int i = 6; i < 9; ++i)
    config.letterToSideMapping[i] = 2;
  for (int i = 9; i < 12; ++i)
    config.letterToSideMapping[i] = 3;

  config.charToIndexMap.fill(-1);
  for (int i = 0; i < 12; ++i) {
    config.charToIndexMap[static_cast<unsigned char>(config.allLetters[i])] = i;
  }

  LetterBoxed::Result result = LetterBoxed::runLetterBoxedSolver(config);

  for (const auto &solution : result.solutions) {
    // Count unique letters used in the solution
    std::bitset<12> usedLetters;
    std::string text = solution.text;
    for (char c : text) {
      if (c != ' ' && c != '+') {
        int idx = config.charToIndexMap[static_cast<unsigned char>(c)];
        if (idx >= 0 && idx < 12) {
          usedLetters.set(idx);
        }
      }
    }
    EXPECT_EQ(usedLetters.count(), 12)
        << "Solution '" << solution.text << "' must use all 12 letters";
  }
}

// =============================================================================
// DUNGLEON TESTS
// =============================================================================

TEST(DungleonTest, ParseFeedback) {
  // Example input: 5 two-letter ids + 5 digits
  std::string input = "ar kn ma bt dr 01234";
  Dungleon::Config config;
  Dungleon::Feedback fb = Dungleon::parseFeedback(input, config);

  // Check pattern characters map to expected enums
  EXPECT_EQ(fb.pattern.characters[0], static_cast<uint8_t>(Dungleon::ARCHER));
  EXPECT_EQ(fb.pattern.characters[1], static_cast<uint8_t>(Dungleon::KNIGHT));
  EXPECT_EQ(fb.pattern.characters[2], static_cast<uint8_t>(Dungleon::MAGE));
  EXPECT_EQ(fb.pattern.characters[3], static_cast<uint8_t>(Dungleon::BAT));
  EXPECT_EQ(fb.pattern.characters[4], static_cast<uint8_t>(Dungleon::DRAGON));

  // Check colors parsed correctly
  EXPECT_EQ(fb.getColor(0), 0);
  EXPECT_EQ(fb.getColor(1), 1);
  EXPECT_EQ(fb.getColor(2), 2);
  EXPECT_EQ(fb.getColor(3), 3);
  EXPECT_EQ(fb.getColor(4), 4);
}

TEST(DungleonTest, GeneratePossiblePatternsNonEmpty) {
  // Should produce a non-empty set of valid possible patterns

  Dungleon::Config config;

  std::vector<Dungleon::Pattern> patterns =
      Dungleon::generateAllPossiblePatterns(config);
  EXPECT_FALSE(patterns.empty())
      << "generateAllPossiblePatterns returned empty";

  // Spot-check that returned patterns are valid according to isValidPattern
  for (size_t i = 0; i < std::min<size_t>(patterns.size(), 10); ++i) {
    EXPECT_TRUE(Dungleon::isValidPattern(patterns[i], config))
        << "Pattern failed validity check: " << patterns[i].toString();
  }
}

TEST(DungleonTest, SolverWithGuesses) {
  Dungleon::Config config;
  config.maxDepth = 0;
  config.excludeImpossiblePatterns = true;

  // Provide one feedback entry to constrain the solver
  // Format: "ar kn ma bt dr 00000"
  config.feedbackHistory.push_back(
      Dungleon::parseFeedback("ar kn ma bt dr 00000", config));

  Dungleon::Result result = Dungleon::runDungleonSolver(config);

  EXPECT_GT(result.sortedGuesses.size(), 0)
      << "Solver should return at least one guess";
  EXPECT_GT(result.totalPossiblePatterns, 0)
      << "There should be at least one possible pattern remaining";
}

TEST(DungleonTest, SolverNoGuesses) {
  // Test solver with no prior guesses
  Dungleon::Config config;
  config.maxDepth = 0;
  config.excludeImpossiblePatterns = false;

  Dungleon::Result result = Dungleon::runDungleonSolver(config);

  EXPECT_GT(result.sortedGuesses.size(), 0) << "Should return suggestions";
  EXPECT_GT(result.totalPossiblePatterns, 0) << "Should have possible patterns";
}

TEST(DungleonTest, SolverWithMultipleFeedback) {
  Dungleon::Config config;
  config.maxDepth = 0;
  config.excludeImpossiblePatterns = true;

  // Add multiple feedback entries
  config.feedbackHistory.push_back(
      Dungleon::parseFeedback("ar kn ma bt dr 00000", config));
  config.feedbackHistory.push_back(
      Dungleon::parseFeedback("vi so bo fr ne 00000", config));

  Dungleon::Result result = Dungleon::runDungleonSolver(config);

  EXPECT_GT(result.sortedGuesses.size(), 0) << "Should return suggestions";
  EXPECT_LT(result.totalPossiblePatterns,
            Dungleon::generateAllPossiblePatterns(config).size())
      << "Multiple feedback should reduce possibilities";
}

TEST(DungleonTest, SolverWithGauntletMode) {
  // Test with past solutions (Gauntlet mode)
  Dungleon::Config config;
  config.maxDepth = 0;
  config.excludeImpossiblePatterns = true;

  // Add a past solution
  Dungleon::Pattern pastSolution;
  pastSolution.characters = {Dungleon::MAGE, Dungleon::KNIGHT,
                             Dungleon::BLADE_ORC, Dungleon::FROG,
                             Dungleon::DRAGON};
  pastSolution.computeCharacterCount();
  config.solutionHistory.push_back(pastSolution);

  Dungleon::Result result = Dungleon::runDungleonSolver(config);

  EXPECT_GT(result.sortedGuesses.size(), 0) << "Should return suggestions";

  // Verify that past solution is not in the possible patterns
  bool foundPastSolution = false;
  for (const auto &guess : result.sortedGuesses) {
    if (guess.pattern == pastSolution && guess.probability > 0.0) {
      foundPastSolution = true;
      break;
    }
  }
  EXPECT_FALSE(foundPastSolution)
      << "Past solution should be excluded from possibilities";
}

TEST(DungleonTest, PatternValidation) {
  Dungleon::Config config;

  // Test valid pattern
  Dungleon::Pattern validPattern;
  validPattern.characters = {Dungleon::MAGE, Dungleon::BAT, Dungleon::AXE_ORC,
                             Dungleon::FROG, Dungleon::BAT};
  validPattern.computeCharacterCount();
  EXPECT_TRUE(Dungleon::isValidPattern(validPattern, config));

  // Pattern should have correct character counts
  EXPECT_EQ(validPattern.characterCount[Dungleon::MAGE], 1);
  EXPECT_EQ(validPattern.characterCount[Dungleon::BAT], 2);
}

TEST(DungleonTest, FeedbackColorEncoding) {
  Dungleon::Feedback fb;
  fb.pattern.characters = {Dungleon::ARCHER, Dungleon::KNIGHT, Dungleon::MAGE,
                           Dungleon::BAT, Dungleon::DRAGON};

  // Test all 5 color values
  fb.setColor(0, 0); // not present
  fb.setColor(1, 1); // diff pos no more
  fb.setColor(2, 2); // correct pos no more
  fb.setColor(3, 3); // diff pos one more
  fb.setColor(4, 4); // correct pos one more

  EXPECT_EQ(fb.getColor(0), 0);
  EXPECT_EQ(fb.getColor(1), 1);
  EXPECT_EQ(fb.getColor(2), 2);
  EXPECT_EQ(fb.getColor(3), 3);
  EXPECT_EQ(fb.getColor(4), 4);
}

TEST(DungleonTest, GenerateFeedbackConsistency) {
  Dungleon::Pattern target;
  target.characters = {Dungleon::MAGE, Dungleon::VILLAGER, Dungleon::BLADE_ORC,
                       Dungleon::FROG, Dungleon::SORCERER};
  target.computeCharacterCount();

  Dungleon::Pattern guess;
  guess.characters = {Dungleon::MAGE, Dungleon::KNIGHT, Dungleon::BLADE_ORC,
                      Dungleon::FROG, Dungleon::FROG};
  guess.computeCharacterCount();

  Dungleon::Feedback fb1 = Dungleon::generateFeedback(target, guess);
  Dungleon::Feedback fb2 = Dungleon::generateFeedback(target, guess);

  // Feedback should be consistent
  EXPECT_EQ(fb1, fb2);
}

TEST(DungleonTest, MatchesFeedbackCorrect) {
  Dungleon::Pattern target;
  target.characters = {Dungleon::MAGE, Dungleon::VILLAGER, Dungleon::BLADE_ORC,
                       Dungleon::FROG, Dungleon::SORCERER};
  target.computeCharacterCount();

  Dungleon::Pattern guess;
  guess.characters = {Dungleon::MAGE, Dungleon::KNIGHT, Dungleon::BLADE_ORC,
                      Dungleon::FROG, Dungleon::FROG};
  guess.computeCharacterCount();

  Dungleon::Feedback fb = Dungleon::generateFeedback(target, guess);
  EXPECT_TRUE(Dungleon::matchesFeedback(target, fb));
}

TEST(DungleonTest, PatternToStringConsistency) {
  Dungleon::Pattern pattern;
  pattern.characters = {Dungleon::ARCHER, Dungleon::KNIGHT, Dungleon::MAGE,
                        Dungleon::BAT, Dungleon::DRAGON};
  pattern.computeCharacterCount();

  std::string str = pattern.toString();
  EXPECT_FALSE(str.empty());

  // Verify it contains expected character IDs
  EXPECT_NE(str.find("ar"), std::string::npos);
  EXPECT_NE(str.find("kn"), std::string::npos);
  EXPECT_NE(str.find("ma"), std::string::npos);
}

// =============================================================================
// PERMUTATION / MATCHING CONSISTENCY TESTS
// For each game, generate feedback for all permutations of targets/guesses
// using the examples provided and verify that matchesFeedback(candidate, fb)
// is equivalent to (generateFeedback(candidate, guess) == fb) for all
// candidate patterns/words.
// =============================================================================

// Helper: build a Utils::Word with letter counts from a lowercase string
static Utils::Word makeWord(const std::string &s) {
  Utils::Word w;
  w.wordString = s;
  w.score = 0.0;
  w.is_scrabble = false;
  w.uniqueLetters = 0;
  w.letterCount.fill(0);
  for (char c : s) {
    if (c >= 'a' && c <= 'z') {
      ++w.letterCount[c - 'a'];
    }
  }
  return w;
}

TEST(WordleTest, PermutationsGenerateAndMatch) {
  // Provided words: TARES, TEETH, EBONY
  std::vector<std::string> wordsStr = {"tares", "teeth", "ebony"};
  std::vector<Utils::Word> words;
  for (auto &w : wordsStr)
    words.push_back(makeWord(w));

  // For every target and guess, generate feedback and verify matching
  for (const auto &target : words) {
    for (const auto &guessStr : wordsStr) {
      Wordle::Feedback fb = Wordle::generateFeedback(target, guessStr);

      for (const auto &candidate : words) {
        Wordle::Feedback fbCandidate =
            Wordle::generateFeedback(candidate, guessStr);
        bool matches = Wordle::matchesFeedback(candidate, fb);
        EXPECT_EQ(matches, (fbCandidate == fb))
            << "Mismatch for target='" << target.wordString << "' guess='"
            << guessStr << "' candidate='" << candidate.wordString << "'";
      }
    }
  }
}

TEST(MastermindTest, PermutationsGenerateAndMatch) {
  // Provided patterns: 1234, 1221, 3153
  Mastermind::Config config;
  config.numPegs = 4;
  config.colorChars = "012345"; // allow digits used in examples
  config.allowDuplicates = true;

  auto makePatternFromStr = [&](const std::string &s) {
    std::array<uint8_t, Mastermind::MAX_PEGS> colors = {};
    uint8_t numPegs = 0;
    for (char c : s) {
      int ci = config.charToColor(c);
      EXPECT_GE(ci, 0) << "Invalid color char in test string: " << c;
      colors[numPegs++] = static_cast<uint8_t>(ci);
    }
    return Mastermind::Pattern(colors, numPegs);
  };

  std::vector<std::string> pats = {"1234", "1221", "3153"};
  std::vector<Mastermind::Pattern> patterns;
  for (auto &p : pats)
    patterns.push_back(makePatternFromStr(p));

  for (const auto &target : patterns) {
    for (const auto &guess : patterns) {
      Mastermind::Feedback fb = Mastermind::generateFeedback(target, guess);

      for (const auto &candidate : patterns) {
        Mastermind::Feedback fbCandidate =
            Mastermind::generateFeedback(candidate, guess);
        bool matches = Mastermind::matchesFeedback(candidate, fb);
        EXPECT_EQ(matches, (fbCandidate == fb))
            << "Mismatch for target='" << target.toString(config) << "' guess='"
            << guess.toString(config) << "' candidate='"
            << candidate.toString(config) << "'";
      }
    }
  }
}

TEST(DungleonTest, PermutationsGenerateAndMatch) {
  // Provided patterns (use enum values from Dungleon::Character)
  // A: MAGE VILLAGER BLADE_ORC FROG SORCERER
  // B: MAGE KNIGHT BLADE_ORC FROG FROG
  // C: MAGE MAGE BLADE_ORC FROG RELIC
  Dungleon::Pattern A;
  A.characters = {Dungleon::MAGE, Dungleon::VILLAGER, Dungleon::BLADE_ORC,
                  Dungleon::FROG, Dungleon::SORCERER};
  A.computeCharacterCount();

  Dungleon::Pattern B;
  B.characters = {Dungleon::MAGE, Dungleon::KNIGHT, Dungleon::BLADE_ORC,
                  Dungleon::FROG, Dungleon::FROG};
  B.computeCharacterCount();

  Dungleon::Feedback expectedFB_A_B;
  expectedFB_A_B.pattern = B;
  expectedFB_A_B.setColor(0, 2);
  expectedFB_A_B.setColor(1, 0);
  expectedFB_A_B.setColor(2, 2);
  expectedFB_A_B.setColor(3, 2);
  expectedFB_A_B.setColor(4, 0);
  Dungleon::Feedback fbAB = Dungleon::generateFeedback(A, B);
  EXPECT_EQ(expectedFB_A_B, fbAB) << "Unexpected feedback for A vs B";

  Dungleon::Pattern C;
  C.characters = {Dungleon::MAGE, Dungleon::MAGE, Dungleon::BLADE_ORC,
                  Dungleon::FROG, Dungleon::RELIC};
  C.computeCharacterCount();

  Dungleon::Feedback expectedFB_B_C;
  expectedFB_B_C.pattern = C;
  expectedFB_B_C.setColor(0, 2);
  expectedFB_B_C.setColor(1, 0);
  expectedFB_B_C.setColor(2, 2);
  expectedFB_B_C.setColor(3, 4);
  expectedFB_B_C.setColor(4, 0);
  Dungleon::Feedback fbBC = Dungleon::generateFeedback(B, C);
  EXPECT_EQ(expectedFB_B_C, fbBC) << "Unexpected feedback for B vs C";

  std::vector<Dungleon::Pattern> patterns = {A, B, C};

  for (const auto &target : patterns) {
    for (const auto &guess : patterns) {
      Dungleon::Feedback fb = Dungleon::generateFeedback(target, guess);

      for (const auto &candidate : patterns) {
        Dungleon::Feedback fbCandidate =
            Dungleon::generateFeedback(candidate, guess);
        bool matches = Dungleon::matchesFeedback(candidate, fb);
        EXPECT_EQ(matches, (fbCandidate == fb))
            << "Mismatch for Dungleon target='" << target.toString()
            << "' guess='" << guess.toString() << "' candidate='"
            << candidate.toString() << "'" << " matches=" << matches;
      }
    }
  }
}

// =============================================================================
// HANGMAN TESTS
// =============================================================================

TEST(HangmanTest, ParsePatternString) {
  std::vector<Hangman::WordPattern> patterns =
      Hangman::parsePatternString("?A?? ??? ?????");

  EXPECT_EQ(patterns.size(), 3);
  EXPECT_EQ(patterns[0].pattern, "?a??");
  EXPECT_EQ(patterns[0].length(), 4);
  EXPECT_EQ(patterns[1].pattern, "???");
  EXPECT_EQ(patterns[1].length(), 3);
  EXPECT_EQ(patterns[2].pattern, "?????");
  EXPECT_EQ(patterns[2].length(), 5);
}

TEST(HangmanTest, PatternToString) {
  std::vector<Hangman::WordPattern> patterns =
      Hangman::parsePatternString("?a?? ??? ?????");

  std::string str = Hangman::patternsToString(patterns);
  EXPECT_EQ(str, "?A?? ??? ?????");
}

TEST(HangmanTest, ParseFeedback) {
  Hangman::Feedback fb1 = Hangman::parseFeedback("a 1");
  EXPECT_EQ(fb1.letter, 'a');
  EXPECT_TRUE(fb1.isInWord);

  Hangman::Feedback fb2 = Hangman::parseFeedback("E 0");
  EXPECT_EQ(fb2.letter, 'e');
  EXPECT_FALSE(fb2.isInWord);
}

TEST(HangmanTest, ParseStrikes) {
  std::vector<Hangman::Feedback> strikes = Hangman::parseStrikes("etxzq");
  EXPECT_EQ(strikes.size(), 5);

  // All strikes should be marked as NOT in word
  for (const auto &fb : strikes) {
    EXPECT_FALSE(fb.isInWord);
    EXPECT_EQ(fb.occurrences, 0);
  }

  EXPECT_EQ(strikes[0].letter, 'e');
  EXPECT_EQ(strikes[1].letter, 't');
  EXPECT_EQ(strikes[2].letter, 'x');
  EXPECT_EQ(strikes[3].letter, 'z');
  EXPECT_EQ(strikes[4].letter, 'q');

  // Test with uppercase
  std::vector<Hangman::Feedback> strikes2 = Hangman::parseStrikes("ABC");
  EXPECT_EQ(strikes2.size(), 3);
  EXPECT_EQ(strikes2[0].letter, 'a');
  EXPECT_EQ(strikes2[1].letter, 'b');
  EXPECT_EQ(strikes2[2].letter, 'c');

  // Test empty string
  std::vector<Hangman::Feedback> strikes3 = Hangman::parseStrikes("");
  EXPECT_EQ(strikes3.size(), 0);
}

TEST(HangmanTest, MatchesPattern) {
  Utils::Word word = makeWord("tares");

  Hangman::WordPattern pattern1;
  pattern1.pattern = "?a???";
  EXPECT_TRUE(Hangman::matchesPattern(word, pattern1));

  Hangman::WordPattern pattern2;
  pattern2.pattern = "?????";
  EXPECT_TRUE(Hangman::matchesPattern(word, pattern2));

  Hangman::WordPattern pattern3;
  pattern3.pattern = "?b???";
  EXPECT_FALSE(Hangman::matchesPattern(word, pattern3));

  Hangman::WordPattern pattern4;
  pattern4.pattern = "????";
  EXPECT_FALSE(Hangman::matchesPattern(word, pattern4));
}

TEST(HangmanTest, MatchesFeedback) {
  Hangman::PhraseSolution phrase;
  phrase.words.push_back(makeWord("tares"));

  Hangman::Feedback fb1;
  fb1.letter = 't';
  fb1.isInWord = true;
  EXPECT_TRUE(Hangman::matchesFeedback(phrase, fb1));

  Hangman::Feedback fb2;
  fb2.letter = 'z';
  fb2.isInWord = false;
  EXPECT_TRUE(Hangman::matchesFeedback(phrase, fb2));

  Hangman::Feedback fb3;
  fb3.letter = 't';
  fb3.isInWord = false;
  EXPECT_FALSE(Hangman::matchesFeedback(phrase, fb3));
}

TEST(HangmanTest, GenerateFeedback) {
  Hangman::PhraseSolution phrase;
  phrase.words.push_back(makeWord("tares"));

  Hangman::Feedback fb1 = Hangman::generateFeedback(phrase, 't');
  EXPECT_EQ(fb1.letter, 't');
  EXPECT_TRUE(fb1.isInWord);
  EXPECT_EQ(fb1.occurrences, 1);

  Hangman::Feedback fb2 = Hangman::generateFeedback(phrase, 'z');
  EXPECT_EQ(fb2.letter, 'z');
  EXPECT_FALSE(fb2.isInWord);
  EXPECT_EQ(fb2.occurrences, 0);
}

TEST(HangmanTest, GetAllLetters) {
  std::vector<char> letters = Hangman::getAllLetters();
  EXPECT_EQ(letters.size(), 26);
  EXPECT_EQ(letters[0], 'a');
  EXPECT_EQ(letters[25], 'z');
}

TEST(HangmanTest, GetAvailableLetters) {
  std::vector<Hangman::Feedback> history = Hangman::parseStrikes("ae");

  std::vector<char> available = Hangman::getAvailableLetters(history);
  EXPECT_EQ(available.size(), 24);

  // 'a' and 'e' should not be in available
  EXPECT_EQ(std::find(available.begin(), available.end(), 'a'),
            available.end());
  EXPECT_EQ(std::find(available.begin(), available.end(), 'e'),
            available.end());
  // 'b' should still be available
  EXPECT_NE(std::find(available.begin(), available.end(), 'b'),
            available.end());
}

TEST(HangmanTest, SolverWithPattern) {
  // Load word list
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  Hangman::Config config;
  config.maxDepth = 0;
  config.wordPatterns = Hangman::parsePatternString("?????");

  Hangman::Result result = Hangman::runHangmanSolver(config);

  EXPECT_GT(result.sortedGuesses.size(), 0) << "Should have letter suggestions";
  EXPECT_GT(result.totalPossibleWords, 0) << "Should have possible words";
}

TEST(HangmanTest, SolverWithRevealedLetters) {
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  Hangman::Config config;
  config.maxDepth = 0;
  config.wordPatterns = Hangman::parsePatternString("?a???");

  Hangman::Result result = Hangman::runHangmanSolver(config);

  // All possible words should have 'a' in position 2
  for (const auto &word : result.possibleWords) {
    EXPECT_EQ(word.wordString[1], 'a')
        << "Word " << word.wordString << " should have 'a' at position 2";
  }
}

TEST(HangmanTest, SolverWithFeedbackHistory) {
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  Hangman::Config config;
  config.maxDepth = 0;
  config.wordPatterns = Hangman::parsePatternString("?????");
  // Add strikes - letters NOT in the word
  config.feedbackHistory = Hangman::parseStrikes("zxq");

  Hangman::Result result = Hangman::runHangmanSolver(config);

  // All possible words should not contain 'z', 'x', or 'q'
  for (const auto &word : result.possibleWords) {
    EXPECT_EQ(word.letterCount['z' - 'a'], 0)
        << "Word " << word.wordString << " should not contain 'z'";
    EXPECT_EQ(word.letterCount['x' - 'a'], 0)
        << "Word " << word.wordString << " should not contain 'x'";
    EXPECT_EQ(word.letterCount['q' - 'a'], 0)
        << "Word " << word.wordString << " should not contain 'q'";
  }
}

TEST(HangmanTest, SolverNoPatterns) {
  Hangman::Config config;
  config.maxDepth = 0;
  // No patterns - should return empty result

  Hangman::Result result = Hangman::runHangmanSolver(config);

  EXPECT_EQ(result.totalPossibleWords, 0);
}

TEST(HangmanTest, WordPatternRevealedLetters) {
  Hangman::WordPattern pattern;
  pattern.pattern = "?a?e?";

  auto revealed = pattern.getRevealedLetters();
  EXPECT_EQ(revealed.size(), 2);
  EXPECT_EQ(revealed[0].first, 1);    // position
  EXPECT_EQ(revealed[0].second, 'a'); // letter
  EXPECT_EQ(revealed[1].first, 3);    // position
  EXPECT_EQ(revealed[1].second, 'e'); // letter
}

TEST(HangmanTest, PhraseSolutionToString) {
  Hangman::PhraseSolution phrase;
  phrase.words.push_back(makeWord("hello"));
  phrase.words.push_back(makeWord("world"));

  EXPECT_EQ(phrase.toString(), "hello world");
}

TEST(HangmanTest, FeedbackEquality) {
  // Test using parseStrikes (all letters NOT in word)
  std::vector<Hangman::Feedback> strikes = Hangman::parseStrikes("ab");
  EXPECT_EQ(strikes[0].letter, 'a');
  EXPECT_EQ(strikes[1].letter, 'b');
  EXPECT_FALSE(strikes[0].isInWord);
  EXPECT_FALSE(strikes[1].isInWord);

  // Test equality
  Hangman::Feedback fb1 = Hangman::parseFeedback("a 0");
  Hangman::Feedback fb2 = Hangman::parseFeedback("a 0");
  Hangman::Feedback fb3 = Hangman::parseFeedback("a 1");
  Hangman::Feedback fb4 = Hangman::parseFeedback("b 0");

  EXPECT_EQ(fb1, fb2);
  EXPECT_FALSE(fb1 == fb3);
  EXPECT_FALSE(fb1 == fb4);

  // Strike should equal parseFeedback with 0
  EXPECT_EQ(strikes[0], fb1);
}