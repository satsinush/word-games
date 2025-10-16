#pragma once

#include <map>
#include <string>
#include <vector>

#include "profilerUtils.hpp"
#include "utils.hpp"

namespace Utils {
namespace Benchmarking {

/**
 * Benchmark types
 */
enum class BenchmarkType {
  RUNTIME,    // Measures execution time
  PERFORMANCE // Measures solving performance (e.g., average guesses)
};

/**
 * Benchmark configuration for different game modes
 */
struct BenchmarkConfig {
  std::string gameMode;
  BenchmarkType type = BenchmarkType::RUNTIME;
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
 * Print benchmark results in a formatted table
 */
void printBenchmarkResults(const BenchmarkResult &result);

/**
 * Parse benchmark arguments from command line
 */
BenchmarkConfig parseBenchmarkArgs(const Utils::Input::CommandArgs &cmdArgs);

} // namespace Benchmarking
} // namespace Utils