#pragma once
#include "../game/Game.hpp"
#include "../letterBoxed/letterBoxed.hpp"
#include "../utils/utils.hpp"
#include <vector>

namespace Game {
class LetterBoxedGame : public IGame {
private:
  const std::vector<Utils::Word> &wordVec;
  int totalLetterCount;

  // UI methods
  void drawPuzzle(const std::array<char, 12> &letters);
  LetterBoxed::Config getConfigFromUser();
  LetterBoxed::Config
  getConfigFromArgs(const std::map<std::string, std::string> &args);
  void printSolutions(const std::vector<LetterBoxed::Solution> &solutions,
                      const int limit = 100);

public:
  explicit LetterBoxedGame(const std::vector<Utils::Word> &words);

  void runCLI() override;
  void runHeadless(const Utils::Input::CommandArgs &cmdArgs) override;
  void runGUI() override;
  std::string getGameName() const override { return "letterboxed"; }

  Utils::Testing::BenchmarkResult
  runBenchmark(const Utils::Testing::BenchmarkConfig &config) override;
};
} // namespace Game