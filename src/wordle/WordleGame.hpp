#pragma once
#include "../game/Game.hpp"
#include "../utils/utils.hpp"
#include "../wordle/wordle.hpp"
#include <vector>

namespace Game {
class WordleGame : public IGame {
private:
  const std::vector<Utils::Word> &wordVec;

  // Helper methods
  Wordle::Config getConfigFromUser();
  Wordle::Config
  getConfigFromArgs(const std::map<std::string, std::string> &args);
  std::vector<Wordle::Feedback> getFeedbackFromUser();
  std::vector<Wordle::Feedback>
  getFeedbackFromArgs(const std::map<std::string, std::string> &args);
  void printResults(const Wordle::Result &result);
  void saveResults(const Wordle::Result &result, const std::string &outputFile);

public:
  explicit WordleGame(const std::vector<Utils::Word> &words);

  void runCLI() override;
  void runHeadless(const Utils::Input::CommandArgs &cmdArgs) override;
  void runGUI() override;
  std::string getGameName() const override { return "wordle"; }

  Utils::Benchmarking::BenchmarkResult
  runBenchmark(const Utils::Benchmarking::BenchmarkConfig &config) override;
};
} // namespace Game