#pragma once
#include "../game/Game.hpp"
#include "../spellingBee/spellingBee.hpp"
#include "../utils/utils.hpp"
#include <vector>

namespace Game {
class SpellingBeeGame : public IGame {
private:
  const std::vector<Utils::Word> &wordVec;

  // UI methods
  void drawPuzzle(const std::array<char, 7> &letters);
  SpellingBee::Config getConfigFromUser();
  SpellingBee::Config
  getConfigFromArgs(const std::map<std::string, std::string> &args);
  void printSolutions(const std::vector<Utils::Word> &solutions);

public:
  explicit SpellingBeeGame(const std::vector<Utils::Word> &words);

  void runCLI() override;
  void runHeadless(const Utils::Input::CommandArgs &cmdArgs) override;
  void runGUI() override;
  std::string getGameName() const override { return "spellingbee"; }

  Utils::Testing::BenchmarkResult
  runBenchmark(const Utils::Testing::BenchmarkConfig &config) override;
};
} // namespace Game