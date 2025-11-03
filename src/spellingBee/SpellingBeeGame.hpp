#pragma once
#include "game/Game.hpp"
#include "spellingBee/spellingBee.hpp"
#include "utils/inputUtils.hpp"
#include "utils/wordUtils.hpp"
#include <vector>

namespace Game {
class SpellingBeeGame : public IGame {
private:
  // UI methods
  void drawPuzzle(const std::array<char, 7> &letters);
  SpellingBee::Config getConfigFromUser();
  SpellingBee::Config
  getConfigFromArgs(const std::map<std::string, std::string> &args);
  void printSolutions(const std::vector<Utils::Word> &solutions);

public:
  explicit SpellingBeeGame();

  void runCLI() override;
  void runHeadless(const Utils::Input::CommandArgs &cmdArgs) override;
  std::string getGameName() const override { return "spellingbee"; }
};
} // namespace Game