#pragma once
#include "game/Game.hpp"
#include "mastermind/mastermind.hpp"
#include "utils/inputUtils.hpp"
#include "utils/wordUtils.hpp"
#include <vector>

namespace Game {
class MastermindGame : public IGame {
private:
  // Helper methods
  Mastermind::Config getConfigFromUser();
  Mastermind::Config
  getConfigFromArgs(const std::map<std::string, std::string> &args);
  std::vector<Mastermind::Feedback>
  getFeedbackFromUser(const Mastermind::Config &config);
  std::vector<Mastermind::Feedback>
  getFeedbackFromArgs(const std::map<std::string, std::string> &args,
                      const Mastermind::Config &config);
  void printResults(const Mastermind::Result &result);
  void saveResults(const Mastermind::Result &result,
                   const std::string &outputFile);

public:
  MastermindGame() = default;

  void runCLI() override;
  void runHeadless(const Utils::Input::CommandArgs &cmdArgs) override;
  void runGUI() override;
  std::string getGameName() const override { return "mastermind"; }
};
} // namespace Game