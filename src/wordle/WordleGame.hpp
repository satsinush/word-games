#pragma once
#include "game/Game.hpp"
#include "utils/inputUtils.hpp"
#include "utils/wordUtils.hpp"
#include "wordle/wordle.hpp"
#include <vector>

namespace Game {
class WordleGame : public IGame {
private:
  // Helper methods
  Wordle::Config getConfigFromUser();
  Wordle::Config
  getConfigFromArgs(const std::map<std::string, std::string> &args);
  std::vector<Wordle::Feedback>
  getFeedbackFromArgs(const std::map<std::string, std::string> &args);
  void printResults(const Wordle::Result &result);
  void saveResults(const Wordle::Result &result, const std::string &outputFile);

public:
  explicit WordleGame();

  void runCLI() override;
  void runHeadless(const Utils::Input::CommandArgs &cmdArgs) override;
  std::string getGameName() const override { return "wordle"; }
};
} // namespace Game