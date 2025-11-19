#pragma once
#include "dungleon/dungleon.hpp"
#include "game/Game.hpp"
#include "utils/inputUtils.hpp"
#include <map>
#include <vector>

namespace Game {
class DungleonGame : public IGame {
private:
  // Helper methods
  Dungleon::Config getConfigFromUser();
  Dungleon::Config
  getConfigFromArgs(const std::map<std::string, std::string> &args);
  std::vector<Dungleon::Feedback>
  getFeedbackFromArgs(const std::map<std::string, std::string> &args);
  std::vector<Dungleon::Pattern>
  getSolutionsFromArgs(const std::map<std::string, std::string> &args);
  Dungleon::Pattern parsePattern(const std::string &input);
  void printResults(const Dungleon::Result &result);
  void saveResults(const Dungleon::Result &result,
                   const std::string &outputFile);

public:
  DungleonGame() = default;

  void runCLI() override;
  void runHeadless(const Utils::Input::CommandArgs &cmdArgs) override;
  std::string getGameName() const override { return "dungleon"; }
};
} // namespace Game
