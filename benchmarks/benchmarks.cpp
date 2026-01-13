#include <benchmark/benchmark.h>

#include "dungleon/DungleonGame.hpp"
#include "dungleon/dungleon.hpp"
#include "hangman/HangmanGame.hpp"
#include "hangman/hangman.hpp"
#include "letterBoxed/LetterBoxedGame.hpp"
#include "letterBoxed/letterBoxed.hpp"
#include "mastermind/MastermindGame.hpp"
#include "mastermind/mastermind.hpp"
#include "spellingBee/SpellingBeeGame.hpp"
#include "spellingBee/spellingBee.hpp"
#include "utils/inputUtils.hpp"
#include "utils/utils.hpp"
#include "wordle/WordleGame.hpp"
#include "wordle/wordle.hpp"

// Display in ms: --benchmark_time_unit=ms

// ============================================================================
// Wordle Benchmarks
// ============================================================================

static void BM_Wordle_Runtime(benchmark::State &state) {
  Wordle::Config config;
  config.maxDepth = 1;
  config.excludeUncommonWords = true;

  std::vector<Wordle::Feedback> feedbackHistory;
  feedbackHistory.push_back(Wordle::parseFeedback("STEAL 20100"));

  config.feedbackHistory = feedbackHistory;

  Utils::loadWords(); // Preload words

  for (auto _ : state) {
    Wordle::Result result = Wordle::runWordleSolver(config);
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_Wordle_Runtime);

// ============================================================================
// Spelling Bee Benchmarks
// ============================================================================

static void BM_SpellingBee_Runtime(benchmark::State &state) {
  SpellingBee::Config config;
  // Use test letters: N H M K A C E
  std::string testLetters = "nhmkace";
  config.allLetters.resize(7);
  for (int i = 0; i < 7; ++i) {
    config.allLetters[i] = testLetters[i];
    config.validLettersMap[static_cast<unsigned char>(testLetters[i])] = true;
  }

  Utils::loadWords(); // Preload words

  for (auto _ : state) {
    SpellingBee::Result result = SpellingBee::runSpellingBeeSolver(config);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_SpellingBee_Runtime);

// ============================================================================
// Letter Boxed Benchmarks
// ============================================================================

static void BM_LetterBoxed_Runtime(benchmark::State &state) {
  LetterBoxed::Config config;
  config.maxDepth = 2;
  config.minWordLength = 3;
  config.minUniqueLetters = 3;

  // Set up test letters: U V J S W I T G E B A C (12 letters, 3 per side)
  std::string testLetters = "uvjswitgebac";

  // Initialize charToIndexMap to -1 (invalid)
  for (int k = 0; k < 256; ++k) {
    config.charToIndexMap[k] = -1;
  }

  for (int j = 0; j < 12; ++j) {
    config.allLetters[j] = testLetters[j];
    config.charToIndexMap[static_cast<unsigned char>(testLetters[j])] = j;
    config.uniquePuzzleLetters.set(j);
  }

  // Set up side mappings (3 letters per side, 4 sides)
  for (int i = 0; i < 3; ++i)
    config.letterToSideMapping[i] = 0;
  for (int i = 3; i < 6; ++i)
    config.letterToSideMapping[i] = 1;
  for (int i = 6; i < 9; ++i)
    config.letterToSideMapping[i] = 2;
  for (int i = 9; i < 12; ++i)
    config.letterToSideMapping[i] = 3;

  Utils::loadWords(); // Preload words

  for (auto _ : state) {
    LetterBoxed::Result result = LetterBoxed::runLetterBoxedSolver(config);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_LetterBoxed_Runtime);

// ============================================================================
// Mastermind Benchmarks
// ============================================================================

static void BM_Mastermind_Runtime(benchmark::State &state) {
  Mastermind::Config config;
  config.numPegs = 5;
  config.colorChars = "01234567"; // 8 colors
  config.allowDuplicates = true;
  config.maxDepth = 1;

  // Parse feedback using parseFeedback function
  std::vector<Mastermind::Feedback> feedback;
  feedback.push_back(Mastermind::parseFeedback("11223 1 2", config));
  feedback.push_back(Mastermind::parseFeedback("34567 1 2", config));

  config.feedbackHistory = feedback;

  for (auto _ : state) {
    Mastermind::Result result = Mastermind::runMastermindSolver(config);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_Mastermind_Runtime);

// ============================================================================
// Dungleon Benchmarks
// ============================================================================

static void BM_Dungleon_Runtime(benchmark::State &state) {
  Dungleon::Config config;
  config.maxDepth = 0;

  // Parse feedback using parseFeedback function
  std::vector<Dungleon::Feedback> feedback;
  feedback.push_back(Dungleon::parseFeedback("ar kn bo ne fr 00010", config));
  feedback.push_back(Dungleon::parseFeedback("vi zo ne sk bt 22120", config));

  config.feedbackHistory = feedback;

  for (auto _ : state) {
    Dungleon::Result result = Dungleon::runDungleonSolver(config);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_Dungleon_Runtime);

// ============================================================================
// Hangman Benchmarks
// ============================================================================

static void BM_Hangman_Runtime(benchmark::State &state) {
  Hangman::Config config;
  config.maxDepth = 1;
  config.excludeUncommonWords = true;

  // Set up a 5-letter word pattern with one revealed letter
  config.wordPatterns = Hangman::parsePatternString("?a???");

  // Add strikes (letters NOT in the word)
  config.feedbackHistory = Hangman::parseStrikes("xyz");

  Utils::loadWords(); // Preload words

  for (auto _ : state) {
    Hangman::Result result = Hangman::runHangmanSolver(config);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_Hangman_Runtime);

static void BM_Hangman_MultiWord_Runtime(benchmark::State &state) {
  Hangman::Config config;
  config.maxDepth = 0;
  config.excludeUncommonWords = true;

  // Set up a multi-word phrase pattern
  config.wordPatterns = Hangman::parsePatternString("???? ??? ?????");

  Utils::loadWords(); // Preload words

  for (auto _ : state) {
    Hangman::Result result = Hangman::runHangmanSolver(config);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_Hangman_MultiWord_Runtime);

// Run the benchmarks
BENCHMARK_MAIN();
