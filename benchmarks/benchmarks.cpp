#include <benchmark/benchmark.h>

#include "letterBoxed/LetterBoxedGame.hpp"
#include "letterBoxed/letterBoxed.hpp"
#include "mastermind/MastermindGame.hpp"
#include "mastermind/mastermind.hpp"
#include "spellingBee/SpellingBeeGame.hpp"
#include "spellingBee/spellingBee.hpp"
#include "utils/inputUtils.hpp"
#include "utils/wordUtils.hpp"
#include "wordle/WordleGame.hpp"
#include "wordle/wordle.hpp"

// Global word list - loaded once for all benchmarks
static std::vector<Utils::Word> g_wordVec;

// Load words once before any benchmarks run
static void LoadWords(const benchmark::State &state) {
  if (g_wordVec.empty()) {
    g_wordVec = Utils::loadWords();
  }
}

// ============================================================================
// Wordle Benchmarks
// ============================================================================

static void BM_Wordle_Runtime(benchmark::State &state) {
  LoadWords(state);

  Wordle::Config config;
  config.maxDepth = 1;
  config.excludeUncommonWords = true;

  std::vector<Wordle::Feedback> feedbackHistory;
  feedbackHistory.push_back(Wordle::parseFeedback("STEAL 20100"));

  for (auto _ : state) {
    Wordle::Result result =
        Wordle::runWordleSolver(g_wordVec, feedbackHistory, config);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_Wordle_Runtime);

// ============================================================================
// Spelling Bee Benchmarks
// ============================================================================

static void BM_SpellingBee_Runtime(benchmark::State &state) {
  LoadWords(state);

  SpellingBee::Config config;
  // Use test letters: N H M K A C E
  std::string testLetters = "nhmkace";
  for (int i = 0; i < 7; ++i) {
    config.allLetters[i] = testLetters[i];
    config.validLettersMap[static_cast<unsigned char>(testLetters[i])] = true;
  }

  for (auto _ : state) {
    std::vector<Utils::Word> solutions =
        SpellingBee::runSpellingBeeSolver(g_wordVec, config);
    benchmark::DoNotOptimize(solutions);
  }
}
BENCHMARK(BM_SpellingBee_Runtime);

// ============================================================================
// Letter Boxed Benchmarks
// ============================================================================

static void BM_LetterBoxed_Runtime(benchmark::State &state) {
  LoadWords(state);

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

  for (auto _ : state) {
    std::vector<LetterBoxed::Solution> solutions =
        LetterBoxed::runLetterBoxedSolver(config, g_wordVec);
    benchmark::DoNotOptimize(solutions);
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

  std::vector<Mastermind::Pattern> allPatterns =
      Mastermind::generateAllPatterns(config);

  // Parse feedback using parseFeedback function
  std::vector<Mastermind::Feedback> feedback;
  feedback.push_back(Mastermind::parseFeedback("11223 1 2", config));
  feedback.push_back(Mastermind::parseFeedback("34567 1 2", config));

  for (auto _ : state) {
    Mastermind::Result result =
        Mastermind::runMastermindSolver(allPatterns, feedback, config);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_Mastermind_Runtime);

// Run the benchmarks
BENCHMARK_MAIN();
