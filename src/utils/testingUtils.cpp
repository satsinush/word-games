#include "testingUtils.hpp"

#include <iomanip>
#include <iostream>

#include "inputUtils.hpp"

namespace Utils {
namespace Testing {

void printBenchmarkResults(const BenchmarkResult &result) {
  std::cout << "\n=== BENCHMARK RESULTS ===\n";
  std::cout << "Game Mode: " << result.gameMode << "\n";

  if (result.iterations > 0) {
    std::cout << "Iterations: " << result.iterations << "\n";
    std::cout << "Total Time: " << std::fixed << std::setprecision(2)
              << result.totalTimeMs << " ms\n";
    std::cout << "Average Time: " << std::fixed << std::setprecision(2)
              << result.averageTimeMs << " ms per iteration\n";
  }

  if (result.totalGames > 0) {
    std::cout << "Games Tested: " << result.totalGames << "\n";
    std::cout << "Average Guesses: " << std::fixed << std::setprecision(3)
              << result.averageGuesses << "\n";
    std::cout << "Min Guesses: " << result.minGuesses << "\n";
    std::cout << "Max Guesses: " << result.maxGuesses << "\n";

    if (result.totalTimeMs > 0) {
      std::cout << "Average Time per Game: " << std::fixed
                << std::setprecision(2)
                << result.totalTimeMs / result.totalGames << " ms\n";
    }
  }

  std::cout << "========================\n\n";
}

BenchmarkConfig parseBenchmarkArgs(const Utils::Input::CommandArgs &cmdArgs) {
  BenchmarkConfig config;
  const auto &args = cmdArgs.flags;

  config.gameMode = Utils::Input::getArgValue(args, "mode", std::string(""));

  // Parse benchmark type
  std::string benchmarkTypeStr =
      Utils::Input::getArgValue(args, "benchmark", std::string("runtime"));
  if (benchmarkTypeStr == "performance") {
    config.type = BenchmarkType::PERFORMANCE;
  } else {
    config.type = BenchmarkType::RUNTIME;
  }

  config.iterations = Utils::Input::getArgValue(args, "iterations", 1);
  // Check for both -v and --verbose
  config.verbose = Utils::Input::getArgValue(args, "v", false) ||
                   Utils::Input::getArgValue(args, "verbose", false);

  return config;
}

} // namespace Testing
} // namespace Utils