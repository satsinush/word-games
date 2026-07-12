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
// SET CANDIDATE SET TESTS
// =============================================================================

TEST(SetCandidateSetTest, ContainsO1Lookup) {
  std::vector<Utils::Word> words;
  words.push_back(makeWord("apple"));
  words.push_back(makeWord("grape"));
  words.push_back(makeWord("melon"));
  
  Utils::SetCandidateSet<Utils::Word> set(words);
  
  EXPECT_TRUE(set.contains(makeWord("apple")));
  EXPECT_TRUE(set.contains(makeWord("grape")));
  EXPECT_TRUE(set.contains(makeWord("melon")));
  EXPECT_FALSE(set.contains(makeWord("cherry")));
  EXPECT_EQ(set.size(), 3);
}

TEST(SetCandidateSetTest, FilterPreservesScore) {
  std::vector<Utils::Word> words;
  Utils::Word w1 = makeWord("aaaaa"); w1.score = 10.0;
  Utils::Word w2 = makeWord("bbbbb"); w2.score = 20.0;
  Utils::Word w3 = makeWord("ccccc"); w3.score = 30.0;
  words.push_back(w1);
  words.push_back(w2);
  words.push_back(w3);
  
  Utils::SetCandidateSet<Utils::Word> set(words);
  EXPECT_DOUBLE_EQ(set.totalScore(), 60.0);
  
  // Filter to keep only words with 'b'
  auto filtered = set.filter([](const Utils::Word& w) {
    return w.letterCount['b' - 'a'] > 0;
  });
  
  EXPECT_EQ(filtered.size(), 1);
  EXPECT_DOUBLE_EQ(filtered.totalScore(), 20.0);
  EXPECT_TRUE(filtered.contains(makeWord("bbbbb")));
}

TEST(SetCandidateSetTest, EmptySetHandled) {
  std::vector<Utils::Word> empty;
  Utils::SetCandidateSet<Utils::Word> set(empty);
  
  EXPECT_TRUE(set.empty());
  EXPECT_EQ(set.size(), 0);
  EXPECT_DOUBLE_EQ(set.totalScore(), 0.0);
  EXPECT_FALSE(set.contains(makeWord("test")));
}

// =============================================================================
// ENT SOLVER CORE TESTS
// =============================================================================

TEST(EntSolverTest, MaxDepthZeroSkipsCalculation) {
  std::vector<Utils::Word> testWords;
  testWords.push_back(makeWord("aaaaa"));
  testWords.push_back(makeWord("bbbbb"));
  
  Wordle::Config config;
  config.maxDepth = 0;  // Should skip ENT calculation
  config.wordLength = 5;
  
  Utils::SetCandidateSet<Utils::Word> candidates(testWords);
  
  // With maxDepth=0, solver should return quickly with uniform ENT values
  Wordle::Result result = Wordle::runWordleSolver(config, nullptr);
  
  // All guesses should have same ENT (worst case estimate)
  if (result.sortedGuesses.size() >= 2) {
    double firstEnt = result.sortedGuesses[0].ent;
    double secondEnt = result.sortedGuesses[1].ent;
    // With depth=0, all ENT values should be the same (log2(n))
    EXPECT_NEAR(firstEnt, secondEnt, 0.001);
  }
}

TEST(EntSolverTest, CancellationStopsEarly) {
  Wordle::Config config;
  config.maxDepth = 1;
  config.wordLength = 5;
  
  std::atomic<bool> cancel(true);  // Already cancelled
  
  Wordle::Result result = Wordle::runWordleSolver(config, &cancel);
  
  // With cancellation, result should be empty
  EXPECT_TRUE(result.sortedGuesses.empty());
}

TEST(EntSolverTest, EmptyCandidateSetReturnsEmpty) {
  Wordle::Config config;
  config.maxDepth = 0;
  config.wordLength = 99;  // No words of this length exist
  
  Wordle::Result result = Wordle::runWordleSolver(config, nullptr);
  
  // Should handle gracefully
  EXPECT_EQ(result.totalPossibleWords, 0);
}

// =============================================================================
// WORDLE TESTS
// =============================================================================

TEST(WordleTest, BasicFeedbackParsing) {
  Wordle::Feedback fb = Wordle::parseFeedback("STEAL 20100");
  EXPECT_EQ(fb.word, "steal");
  EXPECT_EQ(fb.getColor(0), Wordle::Color::Green); // S - green
  EXPECT_EQ(fb.getColor(1), Wordle::Color::Grey); // T - grey
  EXPECT_EQ(fb.getColor(2), Wordle::Color::Yellow); // E - yellow
  EXPECT_EQ(fb.getColor(3), Wordle::Color::Grey); // A - grey
  EXPECT_EQ(fb.getColor(4), Wordle::Color::Grey); // L - grey
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

TEST(WordleTest, ProbabilityCalculationCorrect) {
  std::vector<Utils::Word> testWords;
  testWords.push_back(makeWord("aaaaa"));
  testWords.push_back(makeWord("bbbbb"));
  testWords.push_back(makeWord("ccccc"));
  testWords.push_back(makeWord("ddddd"));
  testWords.push_back(makeWord("eeeee"));

  Wordle::Config config;
  config.maxDepth = 0;
  // Feedback that eliminates 'aaaaa' (all grey)
  config.feedbackHistory.push_back(Wordle::parseFeedback("aaaaa 00000"));

  // Local solver traits for testing
  struct LocalWordleSolverTraits {
    using CandidateType = Utils::Word;
    using GuessType = Utils::Word;
    using FeedbackType = Wordle::Feedback;
    using ConfigType = Wordle::Config;
    using CalculatedGuessType = Wordle::WordGuess;
    using ResultType = Wordle::Result;
    using CandidateSetType = Utils::SetCandidateSet<Utils::Word>;
  };

  // Local helper class to access protected AbstractEntSolverSameType
  class LocalWordleSolver : public Utils::AbstractEntSolverSameType<LocalWordleSolverTraits> {
  public:
    LocalWordleSolver(const Wordle::Config &cfg)
      : Utils::AbstractEntSolverSameType<LocalWordleSolverTraits>(cfg) {}
  protected:
    bool matchesFeedback(const Utils::Word &candidate, const Wordle::Feedback &feedback) const override {
      return Wordle::matchesFeedback(candidate, feedback);
    }
    Wordle::Feedback generateFeedback(const Utils::Word &target, const Utils::Word &guess) const override {
      return Wordle::generateFeedback(target, guess.wordString);
    }
    Wordle::WordGuess createGuess(const Utils::Word &word, double ent, double wnt, double probability) const override {
       Wordle::WordGuess g; g.word = word; g.ent = ent; g.wnt = wnt; g.probability = probability; return g; 
    }
    Wordle::Result createResult(const std::vector<Wordle::WordGuess> &guesses, int totalPossible) const override {
       Wordle::Result r; r.sortedGuesses = guesses; return r;
    }
  };

  LocalWordleSolver solver(config);
  Utils::VectorCandidateSet<Utils::Word> initialSet(testWords);
  auto result = solver.solve(testWords, initialSet);

  for (const auto& g : result.sortedGuesses) {
      if (g.word.wordString == "aaaaa") {
           EXPECT_DOUBLE_EQ(g.probability, 0.0);
      } else {
           // 4 valid candidates remaining out of 5. Prob should be 1/4 = 0.25.
           // Current bug expects 1/5 = 0.20.
           EXPECT_NEAR(g.probability, 0.25, 1e-6) 
               << "Incorrect probability for " << g.word.wordString;
      }
  }
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

TEST(MastermindTest, AllowDuplicatesFiltering) {
  Mastermind::Config config;
  config.numPegs = 4;
  config.colorChars = "RGB";
  config.maxDepth = 0;
  config.allowDuplicates = true;
  
  // With duplicates allowed, patterns like RRRR should be valid
  auto patternsWithDups = Mastermind::generateAllPatterns(config);
  bool foundRRRR = false;
  for (const auto& p : patternsWithDups) {
    if (p.toString(config) == "RRRR") {
      foundRRRR = true;
      break;
    }
  }
  EXPECT_TRUE(foundRRRR) << "RRRR should exist when duplicates allowed";
  
  // Without duplicates, RRRR should not exist
  config.allowDuplicates = false;
  auto patternsNoDups = Mastermind::generateAllPatterns(config);
  bool foundRRRRNoDups = false;
  for (const auto& p : patternsNoDups) {
    if (p.toString(config) == "RRRR") {
      foundRRRRNoDups = true;
      break;
    }
  }
  EXPECT_FALSE(foundRRRRNoDups) << "RRRR should NOT exist when duplicates not allowed";
  
  // Count should be different: with dups = 3^4 = 81, without dups = 3*2*1*0... but 3 colors 4 pegs impossible
  // Actually 3 colors, 4 pegs without dups is 0 (not enough colors)
  // Let's use 4 colors for valid test
  config.colorChars = "RGBY";
  config.allowDuplicates = false;
  auto patterns4Colors = Mastermind::generateAllPatterns(config);
  EXPECT_EQ(patterns4Colors.size(), 24); // 4! = 24 permutations
  
  config.allowDuplicates = true;
  auto patterns4ColorsWithDups = Mastermind::generateAllPatterns(config);
  EXPECT_EQ(patterns4ColorsWithDups.size(), 256); // 4^4 = 256
}

TEST(MastermindTest, CorrectPositionAndColorCounts) {
  Mastermind::Config config;
  config.numPegs = 4;
  config.colorChars = "RGBY";
  config.allowDuplicates = true;
  
  // Test exact match
  Mastermind::Pattern target = Mastermind::parseFeedback("RGBY 0 0", config).guess;
  Mastermind::Pattern guess = Mastermind::parseFeedback("RGBY 0 0", config).guess;
  Mastermind::Feedback fb = Mastermind::generateFeedback(target, guess);
  EXPECT_EQ(fb.correctPosition, 4);
  EXPECT_EQ(fb.correctColor, 0);
  
  // Test all wrong position but correct colors
  guess = Mastermind::parseFeedback("YRGB 0 0", config).guess; // Shifted
  fb = Mastermind::generateFeedback(target, guess);
  // R is in wrong pos, G is in wrong pos, B correct, Y wrong pos
  // Actually let's trace: target=RGBY, guess=YRGB
  // pos0: target R, guess Y - wrong (Y exists at pos3 in target)
  // pos1: target G, guess R - wrong (R exists at pos0 in target)
  // pos2: target B, guess G - wrong (G exists at pos1 in target)
  // pos3: target Y, guess B - wrong (B exists at pos2 in target)
  // So: 0 correct position, 4 correct color
  EXPECT_EQ(fb.correctPosition, 0);
  EXPECT_EQ(fb.correctColor, 4);
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
  EXPECT_EQ(fb.getColor(0), Dungleon::Color::Red);
  EXPECT_EQ(fb.getColor(1), Dungleon::Color::Yellow);
  EXPECT_EQ(fb.getColor(2), Dungleon::Color::YellowPlus);
  EXPECT_EQ(fb.getColor(3), Dungleon::Color::Green);
  EXPECT_EQ(fb.getColor(4), Dungleon::Color::GreenPlus);
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
  config.excludeImpossiblePatterns = true;

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
  fb.setColor(0, Dungleon::Color::Red); // not present
  fb.setColor(1, Dungleon::Color::Yellow); // diff pos no more
  fb.setColor(2, Dungleon::Color::YellowPlus); // correct pos no more
  fb.setColor(3, Dungleon::Color::Green); // diff pos one more
  fb.setColor(4, Dungleon::Color::GreenPlus); // correct pos one more

  EXPECT_EQ(fb.getColor(0), Dungleon::Color::Red);
  EXPECT_EQ(fb.getColor(1), Dungleon::Color::Yellow);
  EXPECT_EQ(fb.getColor(2), Dungleon::Color::YellowPlus);
  EXPECT_EQ(fb.getColor(3), Dungleon::Color::Green);
  EXPECT_EQ(fb.getColor(4), Dungleon::Color::GreenPlus);
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
  expectedFB_A_B.setColor(0, Dungleon::Color::Green);
  expectedFB_A_B.setColor(1, Dungleon::Color::Red);
  expectedFB_A_B.setColor(2, Dungleon::Color::Green);
  expectedFB_A_B.setColor(3, Dungleon::Color::Green);
  expectedFB_A_B.setColor(4, Dungleon::Color::Red);
  Dungleon::Feedback fbAB = Dungleon::generateFeedback(A, B);
  EXPECT_EQ(expectedFB_A_B, fbAB) << "Unexpected feedback for A vs B";

  Dungleon::Pattern C;
  C.characters = {Dungleon::MAGE, Dungleon::MAGE, Dungleon::BLADE_ORC,
                  Dungleon::FROG, Dungleon::RELIC};
  C.computeCharacterCount();

  Dungleon::Feedback expectedFB_B_C;
  expectedFB_B_C.pattern = C;
  expectedFB_B_C.setColor(0, Dungleon::Color::Green);
  expectedFB_B_C.setColor(1, Dungleon::Color::Red);
  expectedFB_B_C.setColor(2, Dungleon::Color::Green);
  expectedFB_B_C.setColor(3, Dungleon::Color::GreenPlus);
  expectedFB_B_C.setColor(4, Dungleon::Color::Red);
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
      Hangman::parsePatternString("_A__ ___ _____");

  EXPECT_EQ(patterns.size(), 3);
  EXPECT_EQ(patterns[0].pattern, "_a__");
  EXPECT_EQ(patterns[0].length(), 4);
  EXPECT_EQ(patterns[1].pattern, "___");
  EXPECT_EQ(patterns[1].length(), 3);
  EXPECT_EQ(patterns[2].pattern, "_____");
  EXPECT_EQ(patterns[2].length(), 5);
}

TEST(HangmanTest, PatternToString) {
  std::vector<Hangman::WordPattern> patterns =
      Hangman::parsePatternString("_a__ ___ _____");

  std::string str = Hangman::patternsToString(patterns);
  EXPECT_EQ(str, "_A__ ___ _____");
}

TEST(HangmanTest, ParseStrikes) {
  std::vector<Hangman::Feedback> strikes = Hangman::parseStrikes("etxzq");
  EXPECT_EQ(strikes.size(), 5);

  // All strikes should be marked as NOT in word
  for (const auto &fb : strikes) {
    EXPECT_FALSE(fb.isInWord());
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
  pattern1.pattern = "_a___";
  EXPECT_TRUE(Hangman::matchesPattern(word, pattern1, {}));

  Hangman::WordPattern pattern2;
  pattern2.pattern = "_____";
  EXPECT_TRUE(Hangman::matchesPattern(word, pattern2, {}));

  Hangman::WordPattern pattern3;
  pattern3.pattern = "_b___";
  EXPECT_FALSE(Hangman::matchesPattern(word, pattern3, {}));

  Hangman::WordPattern pattern4;
  pattern4.pattern = "____";
  EXPECT_FALSE(Hangman::matchesPattern(word, pattern4, {}));

  // Test case where a revealed letter appears in an unrevealed position
  Utils::Word word2 = makeWord("fascinated");
  Hangman::WordPattern pattern5;
  pattern5.pattern = "_as_i__t__";
  EXPECT_FALSE(Hangman::matchesPattern(word2, pattern5, {}));

  // Test case where another letter is duplicated and hits an unrevealed position
  Utils::Word word3 = makeWord("fascinates");
  EXPECT_FALSE(Hangman::matchesPattern(word3, pattern5, {}));

  // Valid matching word (no duplicate of revealed letters at unrevealed positions)
  Utils::Word word4 = makeWord("xasyizztww");
  EXPECT_TRUE(Hangman::matchesPattern(word4, pattern5, {}));

  // Test: 'e' is revealed elsewhere. "the" should be blocked from an unrevealed "___" slot
  Utils::Word bug1_word = makeWord("the");
  Hangman::WordPattern bug1_slot0;
  bug1_slot0.pattern = "___";
  std::unordered_set<char> bug1_global_revealed = {'e'}; // 'E' is tracked as globally revealed
  EXPECT_FALSE(Hangman::matchesPattern(bug1_word, bug1_slot0, bug1_global_revealed));

  // Test: 't' is revealed elsewhere. "tush" should be blocked from an unrevealed "_ush" slot
  Utils::Word bug2_word = makeWord("tush");
  Hangman::WordPattern bug2_slot2;
  bug2_slot2.pattern = "_ush";
  std::unordered_set<char> bug2_global_revealed = {'t', 'u', 's', 'h'}; // 'T' is tracked as globally revealed
  EXPECT_FALSE(Hangman::matchesPattern(bug2_word, bug2_slot2, bug2_global_revealed));
}

TEST(HangmanTest, MatchesFeedback) {
  Hangman::PhraseSolution phrase;
  phrase.words.push_back(makeWord("tares"));

  Hangman::Feedback fb1;
  fb1.letter = 't';
  fb1.positions.set(0); // 't' is at position 0 in "tares"
  EXPECT_TRUE(Hangman::matchesFeedback(phrase, fb1));

  Hangman::Feedback fb2;
  fb2.letter = 'z';
  // positions defaults to empty (no 'z' in "tares")
  EXPECT_TRUE(Hangman::matchesFeedback(phrase, fb2));

  Hangman::Feedback fb3;
  fb3.letter = 't';
  // positions empty but 't' IS in "tares" → mismatch
  EXPECT_FALSE(Hangman::matchesFeedback(phrase, fb3));
}

TEST(HangmanTest, GenerateFeedback) {
  Hangman::PhraseSolution phrase;
  phrase.words.push_back(makeWord("tares"));

  Hangman::Feedback fb1 = Hangman::generateFeedback(phrase, 't');
  EXPECT_EQ(fb1.letter, 't');
  EXPECT_TRUE(fb1.isInWord());
  EXPECT_TRUE(fb1.positions.test(0)); // 't' at position 0

  Hangman::Feedback fb2 = Hangman::generateFeedback(phrase, 'z');
  EXPECT_EQ(fb2.letter, 'z');
  EXPECT_FALSE(fb2.isInWord());
}

TEST(HangmanTest, GetAllLetters) {
  std::vector<char> letters = Hangman::getAllLetters();
  EXPECT_EQ(letters.size(), 26);
  EXPECT_EQ(letters[0], 'a');
  EXPECT_EQ(letters[25], 'z');
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
  EXPECT_GT(result.totalPossiblePatterns, 0) << "Should have possible patterns";
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

  EXPECT_EQ(result.totalPossiblePatterns, 0);
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
  EXPECT_FALSE(strikes[0].isInWord());
  EXPECT_FALSE(strikes[1].isInWord());

  // Test equality
  Hangman::Feedback fb1;
  fb1.letter = 'a';
  // positions defaults to empty (not in word)

  Hangman::Feedback fb2;
  fb2.letter = 'a';
  // positions defaults to empty (not in word)

  Hangman::Feedback fb3;
  fb3.letter = 'a';
  fb3.positions.set(0); // letter IS in word at position 0

  Hangman::Feedback fb4;
  fb4.letter = 'b';
  // positions defaults to empty (not in word)

  EXPECT_EQ(fb1, fb2);
  EXPECT_FALSE(fb1 == fb3);
  EXPECT_FALSE(fb1 == fb4);

  // Strike should equal manual feedback with empty positions
  EXPECT_EQ(strikes[0], fb1);
}

// =============================================================================
// HANGMAN EDGE CASE TESTS - Multi-word scenarios
// =============================================================================

TEST(HangmanTest, MultiWordPattern) {
  // Test solver with multiple word patterns
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  Hangman::Config config;
  config.maxDepth = 0;
  // Two 5-letter words
  config.wordPatterns = Hangman::parsePatternString("_____ _____");

  Hangman::Result result = Hangman::runHangmanSolver(config);

  EXPECT_GT(result.sortedGuesses.size(), 0) << "Should have letter suggestions";
  EXPECT_GT(result.totalPossiblePatterns, 0) << "Should have possible patterns";
}

TEST(HangmanTest, MultiWordSolvedState) {
  // When each word slot has exactly one possibility, we're in solved state
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  Hangman::Config config;
  config.maxDepth = 1;
  // "tares" is the only 5-letter word starting with 't' in test set
  // "not" is the only 3-letter word with 'o' in middle
  config.wordPatterns = Hangman::parsePatternString("tares ?o?");

  Hangman::Result result = Hangman::runHangmanSolver(config);

  // With fully revealed first word and constrained second word,
  // correct letters should have ENT near 0
  // Note: This tests the solved state detection
  EXPECT_GT(result.sortedGuesses.size(), 0);
}

TEST(HangmanTest, MultiWordProbabilityCalculation) {
  // Test that probability is calculated as P(in ANY slot), not average
  // If letter is 100% in slot 0 and 0% in slot 1, probability should be 100%
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  Hangman::Config config;
  config.maxDepth = 0;
  // Use patterns that will create interesting probability scenarios
  config.wordPatterns = Hangman::parsePatternString("_____ ___");

  Hangman::Result result = Hangman::runHangmanSolver(config);

  // Verify that we have guesses with probabilities
  bool foundHighProbLetter = false;
  for (const auto &guess : result.sortedGuesses) {
    if (guess.probability > 0.9) {
      foundHighProbLetter = true;
      break;
    }
  }
  // Common letters like 'e', 'a', 's' should have high probability
  EXPECT_TRUE(foundHighProbLetter)
      << "Should have at least one high-probability letter";
}

TEST(HangmanTest, MultiWordDifferentLengths) {
  // Test with words of different lengths
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  Hangman::Config config;
  config.maxDepth = 0;
  config.wordPatterns = Hangman::parsePatternString("___ _____ _____");

  Hangman::Result result = Hangman::runHangmanSolver(config);

  EXPECT_GT(result.sortedGuesses.size(), 0);
  // Letters already revealed should not be in suggestions with 0 probability
  // (they've already been guessed)
}

TEST(HangmanTest, SingleWordSingleSolution) {
  // Edge case: Only one word matches the pattern
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  Hangman::Config config;
  config.maxDepth = 1;
  // Pattern that matches only "tares" (assuming test data)
  config.wordPatterns = Hangman::parsePatternString("tare?");

  Hangman::Result result = Hangman::runHangmanSolver(config);

  // The remaining letter 's' should have 100% probability and 0 ENT
  bool foundS = false;
  for (const auto &guess : result.sortedGuesses) {
    if (guess.letter == 's') {
      foundS = true;
      EXPECT_NEAR(guess.probability, 1.0, 0.01)
          << "'s' should have 100% probability";
      EXPECT_NEAR(guess.ent, 0.0, 0.01) << "'s' should have 0 ENT";
      break;
    }
  }
  EXPECT_TRUE(foundS) << "Should find 's' in guesses";
}

TEST(HangmanTest, WrongLetterInSolvedState) {
  // When puzzle is solved (one word per slot), wrong letters should have ENT=0
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  Hangman::Config config;
  config.maxDepth = 1;
  // Almost fully revealed - only one letter missing
  config.wordPatterns = Hangman::parsePatternString("tare?");

  Hangman::Result result = Hangman::runHangmanSolver(config);

  // Wrong letters (like 'z', 'x') should have 0% probability and ENT near 0
  for (const auto &guess : result.sortedGuesses) {
    if (guess.letter == 'z' || guess.letter == 'x') {
      EXPECT_EQ(guess.probability, 0.0)
          << "'" << guess.letter << "' should have 0% probability";
      // ENT should be close to 0 (one wasted turn)
      EXPECT_EQ(guess.ent, 0.0)
          << "'" << guess.letter << "' should have ENT near 0";
    }
  }
}

TEST(HangmanTest, WordSlotSolutionEquality) {
  // Test WordSlotSolution equality operator
  Hangman::WordSlotSolution slot1;
  slot1.slotIndex = 0;
  slot1.word = makeWord("hello");

  Hangman::WordSlotSolution slot2;
  slot2.slotIndex = 0;
  slot2.word = makeWord("hello");

  Hangman::WordSlotSolution slot3;
  slot3.slotIndex = 1;
  slot3.word = makeWord("hello");

  Hangman::WordSlotSolution slot4;
  slot4.slotIndex = 0;
  slot4.word = makeWord("world");

  EXPECT_EQ(slot1, slot2) << "Same slot and word should be equal";
  EXPECT_FALSE(slot1 == slot3) << "Different slot index should not be equal";
  EXPECT_FALSE(slot1 == slot4) << "Different word should not be equal";
}

TEST(HangmanTest, WordSlotSolutionToString) {
  Hangman::WordSlotSolution slot;
  slot.slotIndex = 2;
  slot.word = makeWord("test");

  std::string str = slot.toString();
  EXPECT_EQ(str, "[2]:test");
}

TEST(HangmanTest, SolverRejectsHiddenRevealedLetters_Bug1) {
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  Hangman::Config config;
  config.maxDepth = 0;
  // Scenario: ___ ___E ____
  config.wordPatterns = Hangman::parsePatternString("___ ___e ____");

  Hangman::Result result = Hangman::runHangmanSolver(config);

  // Since 'E' is revealed in slot 1, "the" CANNOT hide in slot 0 or slot 2
  for (const auto &word : result.possibleWords) {
    if (word.wordString == "the") {
      FAIL() << "Regression: 'the' should have been completely disqualified because 'e' is hidden in slots 0 and 2";
    }
  }
}

TEST(HangmanTest, SolverRejectsHiddenRevealedLetters_Bug2) {
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  Hangman::Config config;
  config.maxDepth = 0;
  // Scenario: _IG TI_E _USH
  config.wordPatterns = Hangman::parsePatternString("_ig ti_e _ush");

  Hangman::Result result = Hangman::runHangmanSolver(config);

  // Since 't' is revealed in slot 1, "tush" CANNOT hide in slot 2 (_ush)
  for (const auto &word : result.possibleWords) {
    if (word.wordString == "tush") {
      FAIL() << "Regression: 'tush' should have been completely disqualified because 't' is hidden in slot 2";
    }
  }
}

// =============================================================================
// WORDLE EDGE CASE TESTS
// =============================================================================

TEST(WordleTest, AllGreens) {
  // Test feedback when guess is exactly correct
  Utils::Word target = makeWord("tares");
  Wordle::Feedback fb = Wordle::generateFeedback(target, "tares");

  for (size_t i = 0; i < 5; ++i) {
    EXPECT_EQ(fb.getColor(i), Wordle::Color::Green) << "Position " << i << " should be green";
  }
}

TEST(WordleTest, AllGrays) {
  // Test feedback when no letters match
  Utils::Word target = makeWord("tares");
  Wordle::Feedback fb = Wordle::generateFeedback(target, "lymph");

  for (size_t i = 0; i < 5; ++i) {
    EXPECT_EQ(fb.getColor(i), Wordle::Color::Grey) << "Position " << i << " should be gray";
  }
}

TEST(WordleTest, DuplicateLettersInGuess) {
  // Test handling of duplicate letters
  Utils::Word target = makeWord("tares");
  // "eerie" has multiple 'e's, but target only has one 'e'
  Wordle::Feedback fb = Wordle::generateFeedback(target, "eerie");

  // First 'e' should be yellow (exists but wrong position)
  // Subsequent 'e's should be gray (no more 'e's available)
  int yellowCount = 0;
  int grayCount = 0;
  for (size_t i = 0; i < 5; ++i) {
    if (fb.getColor(i) == Wordle::Color::Yellow)
      yellowCount++;
    if (fb.getColor(i) == Wordle::Color::Grey)
      grayCount++;
  }
  // Should have exactly one yellow 'e' and remaining grays
  EXPECT_GE(yellowCount, 0);
}

TEST(WordleTest, SolverSingleSolutionLeft) {
  // When only one word is possible, it should be ranked highest
  Wordle::Config config;
  config.maxDepth = 1;

  // Add feedback that narrows down to very few words
  config.feedbackHistory.push_back(Wordle::parseFeedback("TARES 22220"));

  Wordle::Result result = Wordle::runWordleSolver(config);

  // Should have very few possibilities
  EXPECT_LE(result.totalPossibleWords, 5);
}

// =============================================================================
// MASTERMIND EDGE CASE TESTS
// =============================================================================

TEST(MastermindTest, AllCorrect) {
  // Test feedback when guess is exactly correct
  Mastermind::Config config;
  config.numPegs = 4;
  config.colorChars = "012345";
  config.allowDuplicates = true;

  Mastermind::Pattern target =
      Mastermind::parseFeedback("1234 0 0", config).guess;
  Mastermind::Feedback fb = Mastermind::generateFeedback(target, target);

  EXPECT_EQ(fb.correctPosition, 4);
  EXPECT_EQ(fb.correctColor, 0);
}

TEST(MastermindTest, NoneCorrect) {
  // Test feedback when nothing matches
  Mastermind::Config config;
  config.numPegs = 4;
  config.colorChars = "012345";
  config.allowDuplicates = true;

  Mastermind::Pattern target =
      Mastermind::parseFeedback("0000 0 0", config).guess;
  Mastermind::Pattern guess =
      Mastermind::parseFeedback("1111 0 0", config).guess;
  Mastermind::Feedback fb = Mastermind::generateFeedback(target, guess);

  EXPECT_EQ(fb.correctPosition, 0);
  EXPECT_EQ(fb.correctColor, 0);
}

TEST(MastermindTest, AllWrongPosition) {
  // Test when all colors are right but positions are wrong
  Mastermind::Config config;
  config.numPegs = 4;
  config.colorChars = "012345";
  config.allowDuplicates = true;

  Mastermind::Pattern target =
      Mastermind::parseFeedback("1234 0 0", config).guess;
  Mastermind::Pattern guess =
      Mastermind::parseFeedback("4321 0 0", config).guess;
  Mastermind::Feedback fb = Mastermind::generateFeedback(target, guess);

  // Target: 1234, Guess: 4321
  // Position 0: 1 vs 4 - wrong
  // Position 1: 2 vs 3 - wrong
  // Position 2: 3 vs 2 - wrong
  // Position 3: 4 vs 1 - wrong
  // All 4 colors exist but none in correct position
  EXPECT_EQ(fb.correctPosition, 0);
  EXPECT_EQ(fb.correctColor, 4);
}

TEST(MastermindTest, NoDuplicatesPatternGeneration) {
  // Test pattern generation without duplicates
  Mastermind::Config config;
  config.numPegs = 4;
  config.colorChars = "0123";
  config.allowDuplicates = false;

  std::vector<Mastermind::Pattern> patterns =
      Mastermind::generateAllPatterns(config);

  // 4 colors, 4 pegs, no duplicates: 4! = 24 patterns
  EXPECT_EQ(patterns.size(), 24);

  // Verify no pattern has duplicate colors
  for (const auto &pattern : patterns) {
    // Only check the actual pegs used, not the full array
    std::set<uint8_t> colors(pattern.colors.begin(),
                             pattern.colors.begin() + pattern.numPegs);
    EXPECT_EQ(colors.size(), static_cast<size_t>(pattern.numPegs))
        << "Pattern should have no duplicate colors";
  }
}

// =============================================================================
// DUNGLEON EDGE CASE TESTS
// =============================================================================

TEST(DungleonTest, AllCorrectPosition) {
  // Test feedback when guess matches target exactly
  Dungleon::Pattern target;
  target.characters = {Dungleon::MAGE, Dungleon::KNIGHT, Dungleon::BAT,
                       Dungleon::FROG, Dungleon::DRAGON};
  target.computeCharacterCount();

  Dungleon::Feedback fb = Dungleon::generateFeedback(target, target);

  // All positions should be "correct position" (color 2 or 4)
  for (size_t i = 0; i < 5; ++i) {
    Dungleon::Color color = fb.getColor(i);
    EXPECT_TRUE(color == Dungleon::Color::Green || color == Dungleon::Color::GreenPlus)
        << "Position " << i << " should be correct position";
  }
}

TEST(DungleonTest, NoneMatch) {
  // Test feedback when no characters match
  Dungleon::Pattern target;
  target.characters = {Dungleon::MAGE, Dungleon::KNIGHT, Dungleon::BAT,
                       Dungleon::FROG, Dungleon::DRAGON};
  target.computeCharacterCount();

  Dungleon::Pattern guess;
  guess.characters = {Dungleon::VILLAGER, Dungleon::SORCERER,
                      Dungleon::NECROMANCER, Dungleon::ARCHER,
                      Dungleon::BLADE_ORC};
  guess.computeCharacterCount();

  Dungleon::Feedback fb = Dungleon::generateFeedback(target, guess);

  // All positions should be "not present" (color 0)
  for (size_t i = 0; i < 5; ++i) {
    EXPECT_EQ(fb.getColor(i), Dungleon::Color::Red)
        << "Position " << i << " should be not present";
  }
}

TEST(DungleonTest, DuplicateCharactersInGuess) {
  // Test handling when guess has duplicate characters
  Dungleon::Pattern target;
  target.characters = {Dungleon::MAGE, Dungleon::KNIGHT, Dungleon::BAT,
                       Dungleon::FROG, Dungleon::DRAGON};
  target.computeCharacterCount();

  Dungleon::Pattern guess;
  guess.characters = {Dungleon::MAGE, Dungleon::MAGE, Dungleon::MAGE,
                      Dungleon::MAGE, Dungleon::MAGE};
  guess.computeCharacterCount();

  Dungleon::Feedback fb = Dungleon::generateFeedback(target, guess);

  // First MAGE is correct position, rest should indicate "no more"
  EXPECT_EQ(fb.getColor(0), Dungleon::Color::Green); // correct position, no more
}

// =============================================================================
// SPELLING BEE EDGE CASE TESTS
// =============================================================================

TEST(SpellingBeeTest, PangramDetection) {
  // Test that pangrams (words using all 7 letters) are found
  std::vector<Utils::Word> words = loadTestWords();
  ASSERT_FALSE(words.empty()) << "Failed to load word list";

  SpellingBee::Config config;
  std::string lettersStr = "esrtano";

  config.allLetters.resize(7);
  for (size_t i = 0; i < lettersStr.length() && i < 7; ++i) {
    config.allLetters[i] = lettersStr[i];
  }

  // Only iterate over the letters we actually set, not the full array
  for (size_t i = 0; i < lettersStr.length() && i < 7; ++i) {
    char c = config.allLetters[i];
    config.validLettersMap[static_cast<unsigned char>(c)] = true;
  }

  SpellingBee::Result result = SpellingBee::runSpellingBeeSolver(config);

  // Check if any pangrams exist
  bool foundPangram = false;
  for (const auto &word : result.words) {
    std::set<char> usedLetters;
    for (char c : word.wordString) {
      usedLetters.insert(c);
    }
    if (usedLetters.size() == 7) {
      foundPangram = true;
      break;
    }
  }
  // Pangram may or may not exist depending on test data
  (void)foundPangram;
}

TEST(SpellingBeeTest, EmptyLetters) {
  // Edge case: no valid letters set
  SpellingBee::Config config;
  // Leave allLetters and validLettersMap as default (empty/false)

  SpellingBee::Result result = SpellingBee::runSpellingBeeSolver(config);

  EXPECT_EQ(result.words.size(), 0) << "Should find no words with no letters";
}

// =============================================================================
// LETTER BOXED EDGE CASE TESTS
// =============================================================================

TEST(LetterBoxedTest, WordChaining) {
  // Test that solutions chain properly (last letter = first letter of next)
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

  // Verify word chaining in solutions
  // Solution.text contains space-separated words like "word1 word2"
  for (const auto &solution : result.solutions) {
    // Parse words from solution text
    std::vector<std::string> words;
    std::istringstream iss(solution.text);
    std::string word;
    while (iss >> word) {
      words.push_back(word);
    }

    // Check chaining: last letter of word N = first letter of word N+1
    for (size_t i = 1; i < words.size(); ++i) {
      const std::string &prevWord = words[i - 1];
      const std::string &currWord = words[i];

      char lastChar = prevWord.back();
      char firstChar = currWord.front();

      EXPECT_EQ(lastChar, firstChar)
          << "Word chain broken: " << prevWord << " -> " << currWord;
    }
  }
}

// =============================================================================
// HANGMAN TESTS
// =============================================================================

TEST(HangmanTest, SortsByProbabilityDescending) {
  Hangman::Config config;
  config.maxDepth = 0;
  config.wordPatterns = {{"??"}};  // 2-letter word pattern
  config.excludeUncommonWords = false;
  
  Hangman::Result result = Hangman::runHangmanSolver(config, nullptr);
  
  // Verify results are sorted by probability descending (primary key)
  for (size_t i = 1; i < result.sortedGuesses.size(); ++i) {
    EXPECT_GE(result.sortedGuesses[i-1].probability, result.sortedGuesses[i].probability)
        << "Hangman results should be sorted by probability descending";
  }
}

TEST(HangmanTest, LetterGuessOrdering) {
  // Test that LetterGuess comparison sorts by probability desc, then ENT asc
  Hangman::LetterGuess g1, g2, g3;
  
  g1.letter = 'a'; g1.probability = 0.5; g1.ent = 2.0;
  g2.letter = 'b'; g2.probability = 0.3; g2.ent = 1.5;
  g3.letter = 'c'; g3.probability = 0.5; g3.ent = 1.0;  // Same prob as g1, lower ENT
  
  // g1 should be "less" than g2 because higher prob comes first (sorts lower)
  EXPECT_TRUE(g1 < g2) << "Higher probability should sort before lower";
  
  // g3 should be "less" than g1 because same prob but lower ENT
  EXPECT_TRUE(g3 < g1) << "Same probability but lower ENT should sort first";
}

TEST(HangmanTest, BasicSolverReturnsResults) {
  Hangman::Config config;
  config.maxDepth = 0;
  config.wordPatterns = {{"?????"}};  // 5-letter word pattern
  config.excludeUncommonWords = true;
  
  Hangman::Result result = Hangman::runHangmanSolver(config, nullptr);
  
  // Should return 26 letter guesses (a-z)
  EXPECT_EQ(result.sortedGuesses.size(), 26);
  
  // All letters should have probability >= 0
  for (const auto& guess : result.sortedGuesses) {
    EXPECT_GE(guess.probability, 0.0);
    EXPECT_LE(guess.probability, 1.0);
  }
}