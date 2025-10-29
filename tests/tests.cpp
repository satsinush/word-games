#include <algorithm>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <vector>

#include "dungleon/dungleon.hpp"
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

  // Parse command: wordle --max-depth 1 --guesses "STEAL 20100;CRANE 01002"
  std::map<std::string, std::string> args;
  args["max-depth"] = "1";
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
      SpellingBee::runSpellingBeeSolver(config);

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
      LetterBoxed::runLetterBoxedSolver(config);

  // Verify results
  EXPECT_EQ(solutions.size(), 4) << "Should find exactly 4 solutions";

  // Verify solutions are sorted correctly
  for (size_t i = 1; i < solutions.size(); ++i) {
    EXPECT_TRUE(solutions[i - 1] < solutions[i] ||
                !(solutions[i] < solutions[i - 1]))
        << "Solutions should be sorted";
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
  std::vector<Dungleon::Pattern> patterns =
      Dungleon::generateAllPossiblePatterns();
  EXPECT_FALSE(patterns.empty())
      << "generateAllPossiblePatterns returned empty";

  // Spot-check that returned patterns are valid according to isValidPattern
  for (size_t i = 0; i < std::min<size_t>(patterns.size(), 10); ++i) {
    EXPECT_TRUE(Dungleon::isValidPattern(patterns[i]))
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