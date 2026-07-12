#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "dungleon/dungleon.hpp"
#include "utils/EntSolver.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

namespace Dungleon {
// Define global variables and functions in the .cpp file to ensure a single
// definition
std::array<std::string, NUM_CHARACTERS> CHARACTER_IDS = {
    "ar", "kn", "ma", "bt", "dr", "bo", "ne", "ao", "sk", "sp",
    "bd", "tr", "so", "ki", "vi", "co", "ch", "re", "fr", "zo"};

std::array<std::string, NUM_CHARACTERS> CHARACTER_NAMES = {
    "archer",    "knight",      "mage",     "bat",      "dragon",
    "blade orc", "necromancer", "axe orc",  "skeleton", "spider",
    "bandit",    "troll",       "sorcerer", "king",     "villager",
    "coins",     "chest",       "relic",    "frog",     "zombie"};

CharacterType getCharacterType(uint8_t characterId) {
  if (characterId <= 2)
    return HERO;
  else if (characterId >= 3 && characterId <= 12)
    return MONSTER;
  else if (characterId >= 13 && characterId <= 14)
    return NPC;
  else if (characterId >= 15 && characterId <= 17)
    return TREASURE;
  else
    return OTHER;
}

Feedback parseFeedback(const std::string &input, const Config &config) {
  // Expected format: "ab cd ef gh ij 01234"
  // - 5 two-character pairs (space-separated) representing characters
  // - 5 digits (no spaces) representing colors:
  //   0=not present, 1=different position no more, 2=correct position no more,
  //   3=different position one more, 4=correct position one more

  (void)config; // Unused parameter

  Feedback fb;
  std::istringstream iss(input);
  std::vector<std::string> tokens;
  std::string token; // Read all tokens
  while (iss >> token) {
    tokens.push_back(token);
  }

  // Validate input: should have 6 tokens (5 character pairs + 1 color string)
  if (tokens.size() != 6) {
    throw std::runtime_error("Invalid format. Expected: 'ab cd ef gh ij 01234' "
                             "(5 character pairs and 5 color digits)");
  }

  // Parse the 5 character pairs into an array
  std::array<uint8_t, 5> characters = {};
  for (size_t i = 0; i < NUM_SLOTS; ++i) {
    const std::string &charPair = tokens[i];
    if (charPair.length() != 2) {
      throw std::runtime_error(
          "Invalid character pair '" + charPair +
          "'. Each character must be exactly 2 characters.");
    }

    // Find the character ID from CHARACTER_IDS array
    bool found = false;
    for (uint8_t j = 0; j < NUM_CHARACTERS; ++j) {
      if (CHARACTER_IDS[j] == charPair) {
        characters[i] = j;
        found = true;
        break;
      }
    }

    if (!found) {
      throw std::runtime_error(
          "Unknown character '" + charPair +
          "'. Valid characters: ar, kn, ma, bt, dr, bo, ne, ao, sk, sp, bd, "
          "tr, so, ki, vi, co, ch, re, fr, zo");
    }
  }

  // Create pattern using array constructor which automatically calls
  // computeCharacterCount()
  fb.pattern = Pattern(characters);

  // Parse the color string (5 digits)
  const std::string &colorStr = tokens[5];
  if (colorStr.length() != NUM_SLOTS) {
    throw std::runtime_error("Invalid color string '" + colorStr +
                             "'. Must be exactly 5 digits (0-4)");
  }

  for (size_t i = 0; i < NUM_SLOTS; ++i) {
    char colorChar = colorStr[i];
    if (colorChar >= '0' && colorChar <= '4') {
      fb.setColor(i, static_cast<Color>(colorChar - '0'));
    } else {
      throw std::runtime_error(
          "Invalid color digit '" + std::string(1, colorChar) +
          "'. Must be 0-4 (0=not present, 1=diff pos no more, "
          "2=diff pos one more, 3=correct pos no more, 4=correct pos one "
          "more)");
    }
  }

  return fb;
}

// Helper: Check if a pattern matches all feedback constraints
bool matchesFeedback(const Pattern &candidate, const Feedback &fb) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif

  const Pattern &guess = fb.pattern;

  // Use precomputed character counts from the candidate
  std::array<uint8_t, NUM_CHARACTERS> remainingCount = candidate.characterCount;

  // First pass: Check all positions marked as correct (3 or 4)
  for (size_t i = 0; i < NUM_SLOTS; ++i) {
    Color color = fb.getColor(i);
    uint8_t guessChar = guess.characters[i];
    if (color == Color::Green || color == Color::GreenPlus) {
      // Must match at this position
      if (candidate.characters[i] != guessChar) {
        return false;
      }
      if (color == Color::Green && candidate.characterCount[guessChar] >
                            fb.pattern.characterCount[guessChar]) {
        return false; // Too many instances in the candidate
      }
      if (color == Color::GreenPlus && candidate.characterCount[guessChar] <=
                            fb.pattern.characterCount[guessChar]) {
        return false; // Not enough instances in the candidate
      }
      remainingCount[candidate.characters[i]]--;
    }
  }

  // Second pass: Check other constraints
  for (size_t i = 0; i < NUM_SLOTS; ++i) {
    Color color = fb.getColor(i);
    uint8_t guessChar = guess.characters[i];

    if (color == Color::Green || color == Color::GreenPlus) {
      // Already handled in first pass
      continue;
    } else if (color == Color::Yellow || color == Color::YellowPlus) {
      // Character is in the pattern but not at this position
      if (candidate.characters[i] == guessChar) {
        return false; // Cannot be in the same spot
      }
      if (remainingCount[guessChar] <= 0) {
        return false; // Must have this character elsewhere
      }
      if (color == Color::Yellow && candidate.characterCount[guessChar] >
                            fb.pattern.characterCount[guessChar]) {
        return false; // Too many instances in the candidate
      }
      if (color == Color::YellowPlus && candidate.characterCount[guessChar] <=
                            fb.pattern.characterCount[guessChar]) {
        return false; // Not enough instances in the candidate
      }
      remainingCount[guessChar]--;
    } else if (color == Color::Red) {
      // Character is not present (or all instances already accounted for)
      if (remainingCount[guessChar] > 0) {
        return false; // Candidate has more of this character than allowed
      }
    }
  }

  return true;
}

// Generate feedback for a guess against a target pattern
Feedback generateFeedback(const Pattern &target, const Pattern &guess) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  Feedback fb;
  fb.pattern = guess;

  // Use precomputed character counts from the target
  std::array<uint8_t, NUM_CHARACTERS> remainingCount = target.characterCount;

  // First pass: Determine correct positions
  for (size_t i = 0; i < NUM_SLOTS; ++i) {
    uint8_t guessChar = guess.characters[i];

    if (target.characters[i] == guessChar) {
      // Correct position
      if (target.characterCount[guessChar] > guess.characterCount[guessChar]) {
        fb.setColor(i, Color::GreenPlus); // Correct position, more in the target
      } else {
        fb.setColor(i, Color::Green); // Correct position, no more in the target
      }
      remainingCount[guessChar]--; // Decrement remaining count
    }
  }

  // Second pass: Determine full feedback for each position
  for (size_t i = 0; i < NUM_SLOTS; ++i) {
    uint8_t guessChar = guess.characters[i];

    if (target.characters[i] == guessChar) {
      // Correct position
      continue; // Already handled in first pass
    } else {
      // Not in correct position
      if (remainingCount[guessChar] == 0) {
        fb.setColor(i, Color::Red); // No more instances in target
      } else {
        if (target.characterCount[guessChar] >
            guess.characterCount[guessChar]) {
          fb.setColor(i, Color::YellowPlus); // Different position, more in target
        } else {
          fb.setColor(i, Color::Yellow); // Different position, no more in target
        }
        remainingCount[guessChar]--; // Decrement remaining count
      }
    }
  }

  return fb;
}

// Check if a pattern is valid according to game rules
// TODO: Optimize and ensure checks for transformed characters is accurate
bool isValidPattern(const Pattern &pattern, const Config &config,
                    uint8_t numSlots) {
  std::array<uint8_t, NUM_CHARACTER_TYPES> characterTypeCounts = {};
  bool dragonNotInLast = false;

  for (uint8_t i = 0; i < numSlots; ++i) {
    uint8_t c = pattern.characters[i];
    CharacterType cType = getCharacterType(c);
    characterTypeCounts[cType]++;

    if (c == DRAGON && i != 4) {
      dragonNotInLast = true;
    }
  }

  const auto &characterCounts = pattern.characterCount;

  // Position-based constraints
  for (uint8_t i = 0; i < numSlots; ++i) {
    uint8_t c = pattern.characters[i];
    CharacterType cType = getCharacterType(c);

    // Heroes can appear in spots 0 and 1
    if ((cType == HERO || c == ZOMBIE) && !(i == 0 || i == 1)) {
      return false;
    }
    // The Knight always faces a monster
    if (i > 0 && pattern.characters[i - 1] == KNIGHT &&
        !(cType == MONSTER || c == FROG)) {
      return false;
    }
    // The Archer cannot face a monster
    if (i > 0 && pattern.characters[i - 1] == ARCHER &&
        (cType == MONSTER || c == FROG)) {
      return false;
    }
    // The bandit can only appear in the first position
    if (c == BANDIT && i != 0) {
      return false;
    }
    // The sorcerer can only appear in the last position
    if (c == SORCERER && i != 4) {
      return false;
    }
    // The troll always faces a hero or NPC
    if (i > 0) {
      uint8_t lastC = pattern.characters[i - 1];
      CharacterType lastCType = getCharacterType(lastC);
      if (c == TROLL &&
          !(lastCType == HERO || lastCType == NPC || lastC == ZOMBIE)) {
        return false;
      }
    }
    // The villager can only appear in spots 0 or 1
    if (c == VILLAGER && !(i == 0 || i == 1)) {
      return false;
    }
    // The king can only appear in spot 0
    if (c == KING && i != 0) {
      return false;
    }
    // The relic can only appear in spot 4
    if (c == RELIC && i != 4) {
      return false;
    }
  }

  if (numSlots < NUM_SLOTS) {
    return true; // Skip count-based constraints for partial patterns
  }

  // Pattern can't be a past solution
  for (const Pattern &pastSolution : config.solutionHistory) {
    if (pattern == pastSolution) {
      return false;
    }
  }

  // All solutions must have at least one character in common
  if (!config.solutionHistory.empty()) {
    bool hasSharedCharacter = false;
    for (uint8_t c = 0; c < NUM_CHARACTERS; ++c) {
      if (config.sharedCharacters[c] && pattern.characterCount[c] > 0) {
        hasSharedCharacter = true;
        break;
      }
    }
    if (!hasSharedCharacter) {
      return false;
    }
  }

  uint8_t countedFrogs = 0; // Used to count the number of frogs used as
                            // transformations, if this number is greater than
                            // the number of frogs, the pattern is invalid

  // Count-based constraints
  // The Mage always turns a monster into a frog
  if (characterCounts[MAGE] > 0 && characterCounts[FROG] == 0) {
    return false;
  }
  // Bats always come in a pair
  countedFrogs +=
      characterCounts[BAT] % 2 != 0 ? 1 : 0; // Frogs needed to balance
  countedFrogs += characterCounts[SPIDER] % 3 != 0
                      ? (3 - (characterCounts[SPIDER] % 3))
                      : 0; // Frogs needed to balance
  // Axe Orcs and Blade Orcs always appear together
  countedFrogs += std::abs(int(characterCounts[AXE_ORC]) -
                           int(characterCounts[BLADE_ORC])); // Frogs needed to
                                                             // balance
  // The necromancer always turns all heroes into zombies
  if (characterCounts[NECROMANCER] > 0 && characterTypeCounts[HERO] > 0) {
    return false;
  }
  // The troll always comes without treasure
  if (characterCounts[TROLL] > 0 && characterTypeCounts[TREASURE] > 0) {
    return false;
  }
  // The dragon always comes with no other monsters
  if (characterCounts[DRAGON] > 0 &&
      (characterTypeCounts[MONSTER] > 1 || characterCounts[FROG] > 0)) {
    return false;
  }
  // The dragon always comes with no coins
  if (characterCounts[DRAGON] > 0 && characterCounts[COINS] > 0) {
    return false;
  }
  // Coins come as a pair or triple
  if (characterCounts[COINS] > 0 && characterCounts[COINS] % 2 != 0 &&
      characterCounts[COINS] % 3 != 0) {
    return false;
  }
  // If there is a dragon not in the last position, then there is a relic
  if (dragonNotInLast && characterCounts[RELIC] == 0) {
    return false;
  }

  // Ensure we do not exceed available frogs for transformations
  if (countedFrogs > characterCounts[FROG]) {
    return false;
  }

  return true;
}

// Generate all possible patterns for the given configuration
std::vector<Pattern> generateAllPatterns() {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  std::vector<Pattern> patterns;

  // Generate all possible combinations with repetition
  std::array<uint8_t, 5> characters = {};

  std::function<void(unsigned int)> generate = [&](unsigned int pos) {
    if (pos == NUM_SLOTS) {
      // Create pattern using array constructor which automatically calls
      // computeCharacterCount()
      patterns.emplace_back(characters);
      return;
    }

    for (unsigned int c = 0; c < NUM_CHARACTERS; ++c) {
      characters[pos] = static_cast<uint8_t>(c);
      generate(pos + 1);
    }
  };

  generate(0);

  return patterns;
}

// Generate all possible patterns for the given configuration
std::vector<Pattern> generateAllPossiblePatterns(const Config &config) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  std::vector<Pattern> patterns;

  // Generate all possible combinations with repetition
  std::array<uint8_t, 5> characters = {};

  std::function<void(unsigned int)> generate = [&](unsigned int pos) {
    if (pos == NUM_SLOTS) {

      Pattern pattern(characters);

      if (!isValidPattern(pattern, config, pos)) {
        return;
      }

      patterns.push_back(pattern);
      return;
    }

    for (uint8_t c = 0; c < NUM_CHARACTERS; ++c) {
      if (!isValidPattern(Pattern(characters), config, pos)) {
        continue;
      }

      characters[pos] = static_cast<uint8_t>(c);
      generate(pos + 1);
    }
  };

  generate(0);

  return patterns;
}

// Dungleon solver traits
struct DungleonSolverTraits {
  using CandidateType = Pattern;
  using GuessType = Pattern;
  using FeedbackType = Feedback;
  using ConfigType = Config;
  using CalculatedGuessType = PatternGuess;
  using ResultType = Result;
  using CandidateSetType = Utils::SetCandidateSet<Pattern>;
};

// Dungleon-specific ENT solver implementation
class DungleonEntSolver : public Utils::AbstractEntSolverSameType<DungleonSolverTraits> {
public:
  DungleonEntSolver(const Config &cfg)
      : Utils::AbstractEntSolverSameType<DungleonSolverTraits>(cfg) {}

protected:
  bool matchesFeedback(const Pattern &candidate,
                       const Feedback &feedback) const override {
    return Dungleon::matchesFeedback(candidate, feedback);
  }

  Feedback generateFeedback(const Pattern &target,
                            const Pattern &guess) const override {
    return Dungleon::generateFeedback(target, guess);
  }

  PatternGuess createGuess(const Pattern &pattern, double ent, double wnt, double probability) const override {
    PatternGuess guess;
    guess.pattern = pattern;
    guess.ent = ent;
    guess.wnt = wnt;
    guess.probability = probability;
    return guess;
  }

  Result createResult(const std::vector<PatternGuess> &guesses,
                      int totalPossible) const override {
    Result result;
    result.sortedGuesses = guesses;
    result.totalPossiblePatterns = totalPossible;
    result.searchDepth = this->activeDepth;
    return result;
  }

  double maxFeedbackGroups() const override {
    // 5 feedback states per slot, NUM_SLOTS slots
    double k = 1.0;
    for (uint8_t i = 0; i < NUM_SLOTS; ++i)
      k *= 5.0;
    return k; // 3125
  }

  double feedbackEfficiency() const override {
    return 0.60; // Highly constrained character pairing rules
  }
};

Result runDungleonSolver(const Config &_config, std::atomic<bool> *cancel) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif

  Config config = _config; // Make a copy to modify
  if (!config.solutionHistory.empty()) {
    config.sharedCharacters.fill(true);
    for (const Pattern &p : config.solutionHistory) {
      for (uint8_t c = 0; c < NUM_CHARACTERS; ++c) {
        if (p.characterCount[c] == 0) {
          config.sharedCharacters[c] = false;
        }
      }
    }
  }

  std::vector<Pattern> possiblePatterns = generateAllPossiblePatterns(config);

  std::vector<Pattern> filteredPossible = possiblePatterns;
  std::vector<Pattern> filteredAll;
  if (config.excludeImpossiblePatterns) {
    filteredAll = filteredPossible;
  } else {
    filteredAll = generateAllPatterns();
  }

  // Create CandidateSet from the filtered patterns
  Utils::VectorCandidateSet<Dungleon::Pattern> initialCandidates(filteredPossible);

  // Use the specialized Dungleon ENT solver - returns Result directly!
  DungleonEntSolver solver(config);
  return solver.solve(filteredAll, initialCandidates, cancel);
}
} // namespace Dungleon