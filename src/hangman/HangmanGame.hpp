#pragma once
#include "game/Game.hpp"
#include "hangman/hangman.hpp"
#include "utils/inputUtils.hpp"
#include "utils/utils.hpp"
#include <vector>

namespace Game {
class HangmanGame : public IGame {
private:
  // Helper methods
  Hangman::Config getConfigFromUser();
  Hangman::Config
  getConfigFromArgs(const std::map<std::string, std::string> &args);
  std::vector<Hangman::Feedback>
  getFeedbackFromArgs(const std::map<std::string, std::string> &args);
  void printResults(const Hangman::Result &result);
  void saveResults(const Hangman::Result &result,
                   const std::string &outputFile);

public:
  explicit HangmanGame();

  void runCLI() override;
  void runHeadless(const Utils::Input::CommandArgs &cmdArgs) override;
  std::string getGameName() const override { return "hangman"; }
};
} // namespace Game
