#include <algorithm>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <vector>

#include "letterBoxed/letterBoxed.hpp"
#include "mastermind/mastermind.hpp"
#include "spellingBee/spellingBee.hpp"
#include "utils/inputUtils.hpp"
#include "utils/wordUtils.hpp"
#include "wordle/wordle.hpp"

// Helper function to load words for testing
std::vector<Utils::Word> loadTestWords() {
  // Load from test CSV without binary cache, load all words
  std::vector<Utils::Word> words =
      Utils::loadWords("resources/test_word_scores.csv", false, 0);
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

  // Parse command: wordle --max-depth 1 --guesses "STEAL 20100;CRANE 01002"
  std::map<std::string, std::string> args;
  args["max-depth"] = "1";
  args["guesses"] = "STEAL 20100;CRANE 01002";

  // Build config
  Wordle::Config config;
  config.maxDepth = std::stoi(args["max-depth"]);

  // Parse feedback history
  std::vector<Wordle::Feedback> feedbackHistory;
  std::string guessesStr = args["guesses"];
  size_t pos = 0;
  while ((pos = guessesStr.find(';')) != std::string::npos) {
    std::string token = guessesStr.substr(0, pos);
    feedbackHistory.push_back(Wordle::parseFeedback(token));
    guessesStr.erase(0, pos + 1);
  }
  if (!guessesStr.empty()) {
    feedbackHistory.push_back(Wordle::parseFeedback(guessesStr));
  }

  EXPECT_EQ(feedbackHistory.size(), 2);

  // Run solver
  Wordle::Result result =
      Wordle::runWordleSolver(words, feedbackHistory, config);

  // Verify results
  EXPECT_GT(result.sortedGuesses.size(), 0) << "Should have at least one guess";
  EXPECT_EQ(result.totalPossibleWords, 2)
      << "Should have exactly 2 possible words remaining";
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
  //                --max-depth 1
  Mastermind::Config config;
  config.numPegs = 4;
  config.colorChars = "012345"; // 6 colors
  config.allowDuplicates = true;
  config.maxDepth = 1;

  // Generate all patterns
  std::vector<Mastermind::Pattern> allPatterns =
      Mastermind::generateAllPatterns(config);
  EXPECT_EQ(allPatterns.size(), 1296);

  // Parse guess history using parseFeedback
  std::vector<Mastermind::Feedback> guessHistory;

  // First guess: "1122 1 2"
  guessHistory.push_back(Mastermind::parseFeedback("1122 1 2", config));

  // Second guess: "2131 2 1"
  guessHistory.push_back(Mastermind::parseFeedback("2131 2 1", config));

  // Run solver
  Mastermind::Result result =
      Mastermind::runMastermindSolver(allPatterns, guessHistory, config);

  // Verify results
  EXPECT_GT(result.sortedGuesses.size(), 0) << "Should have at least one guess";
  EXPECT_GT(result.totalPossiblePatterns, 0)
      << "Should have possible patterns remaining";

  // TODO: Add specific expected values when you provide them
  // EXPECT_EQ(result.totalPossiblePatterns, EXPECTED_VALUE);
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
    config.allLetters[i] = lettersStr[i];
  }

  // Set up valid letters map
  for (char c : config.allLetters) {
    config.validLettersMap[static_cast<unsigned char>(c)] = true;
  }

  // Run solver
  std::vector<Utils::Word> solutions =
      SpellingBee::runSpellingBeeSolver(words, config);

  // Verify results
  EXPECT_EQ(solutions.size(), 4) << "Should find exactly 4 solutions";

  // Verify all solutions only use allowed letters
  for (const auto &word : solutions) {
    for (char c : word.wordString) {
      EXPECT_TRUE(config.validLettersMap[static_cast<unsigned char>(c)])
          << "Word '" << word.wordString << "' contains invalid letter '" << c
          << "'";
    }
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
  std::vector<LetterBoxed::Solution> solutions =
      LetterBoxed::runLetterBoxedSolver(config, words);

  // Verify results
  EXPECT_EQ(solutions.size(), 4) << "Should find exactly 4 solutions";

  // Verify solutions are sorted correctly
  for (size_t i = 1; i < solutions.size(); ++i) {
    EXPECT_TRUE(solutions[i - 1] < solutions[i] ||
                !(solutions[i] < solutions[i - 1]))
        << "Solutions should be sorted";
  }
}