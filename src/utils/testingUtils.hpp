#pragma once

#include <map>
#include <string>
#include <vector>

#include "profilerUtils.hpp"
#include "utils.hpp"

namespace Utils {
namespace Testing {

/**
 * Benchmark configuration for different game modes
 */
struct BenchmarkConfig {
  std::string gameMode;
  int iterations = 1;
  bool verbose = false;
};

/**
 * Results from a benchmark run
 */
struct BenchmarkResult {
  double totalTimeMs = 0.0;
  double averageTimeMs = 0.0;
  int iterations = 0;
  std::string gameMode;

  // Performance-specific results (for Wordle)
  double averageGuesses = 0.0;
  int totalGames = 0;
  int maxGuesses = 0;
  int minGuesses = 0;
};

/**
 * Run a basic runtime benchmark for the specified game mode
 */
BenchmarkResult
runRuntimeBenchmark(const std::string &gameMode,
                    const std::vector<Utils::Word> &wordVec,
                    const BenchmarkConfig &config = BenchmarkConfig{});

/**
 * Run a performance benchmark (currently only available for Wordle)
 * Tests the solver against a set of target words and measures solving
 * performance
 */
BenchmarkResult
runPerformanceBenchmark(const std::string &gameMode,
                        const std::vector<Utils::Word> &wordVec,
                        const BenchmarkConfig &config = BenchmarkConfig{});

/**
 * Print benchmark results in a formatted table
 */
void printBenchmarkResults(const BenchmarkResult &result);

/**
 * Parse benchmark arguments from command line
 */
BenchmarkConfig
parseBenchmarkArgs(const std::map<std::string, std::string> &args);

} // namespace Testing
} // namespace Utils