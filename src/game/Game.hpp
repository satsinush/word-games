#pragma once
#include "utils/inputUtils.hpp"
#include <string>

namespace Game {
// Base interface for all games
class IGame {
public:
  virtual ~IGame() = default;

  // Run game in interactive command-line mode
  virtual void runCLI() = 0;

  // Run game in headless mode with command-line arguments
  virtual void runHeadless(const Utils::Input::CommandArgs &cmdArgs) = 0;

  // Run game with GUI (to be implemented later)
  virtual void runGUI() = 0;

  // Get game name for identification
  virtual std::string getGameName() const = 0;
};
} // namespace Game