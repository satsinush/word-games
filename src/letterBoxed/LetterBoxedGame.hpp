#pragma once
#include "game/Game.hpp"
#include "letterBoxed/letterBoxed.hpp"
#include "utils/inputUtils.hpp"
#include "utils/utils.hpp"
#include <vector>

namespace Game {
class LetterBoxedGame : public IGame {
private:
  // UI methods
  void drawPuzzle(const std::array<char, 12> &letters);
  LetterBoxed::Config getConfigFromUser();
  LetterBoxed::Config
  getConfigFromArgs(const std::map<std::string, std::string> &args);
  void printSolutions(const std::vector<LetterBoxed::Solution> &solutions,
                      const int limit = 100);

public:
  explicit LetterBoxedGame();

  void runCLI() override;
  void runHeadless(const Utils::Input::CommandArgs &cmdArgs) override;
  std::string getGameName() const override { return "letterboxed"; }
};
} // namespace Game