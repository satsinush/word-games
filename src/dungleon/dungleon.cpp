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
      fb.setColor(i, colorChar - '0');
    } else {
      throw std::runtime_error(
          "Invalid color digit '" + std::string(1, colorChar) +
          "'. Must be 0-4 (0=not present, 1=diff pos no more, "
          "2=correct pos no more, 3=diff pos one more, 4=correct pos one "
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
  // First check if the pattern is valid according to game rules
  if (!isValidPattern(candidate)) {
    return false;
  }

  const Pattern &guess = fb.pattern;

  // Use precomputed character counts from the candidate
  std::array<uint8_t, NUM_CHARACTERS> candidateCount = candidate.characterCount;

  // First pass: Check all positions marked as correct (2 or 4)
  for (size_t i = 0; i < NUM_SLOTS; ++i) {
    int color = fb.getColor(i);
    if (color == 2 || color == 4) {
      // Must match at this position
      if (candidate.characters[i] != guess.characters[i]) {
        return false;
      }
      candidateCount[candidate.characters[i]]--;
    }
  }

  // Second pass: Check other constraints
  for (size_t i = 0; i < NUM_SLOTS; ++i) {
    int color = fb.getColor(i);
    uint8_t guessChar = guess.characters[i];

    if (color == 2 || color == 4) {
      // Already handled in first pass
      continue;
    } else if (color == 1 || color == 3) {
      // Character is in the pattern but not at this position
      if (candidate.characters[i] == guessChar) {
        return false; // Cannot be in the same spot
      }
      if (candidateCount[guessChar] <= 0) {
        return false; // Must have this character elsewhere
      }
      candidateCount[guessChar]--;
    } else if (color == 0) {
      // Character is not present (or all instances already accounted for)
      if (candidateCount[guessChar] > 0) {
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

  // First pass: Mark correct positions (2 or 4)
  for (size_t i = 0; i < NUM_SLOTS; ++i) {
    if (target.characters[i] == guess.characters[i]) {
      remainingCount[target.characters[i]]--;
    }
  }

  // Second pass: Determine full feedback for each position
  for (size_t i = 0; i < NUM_SLOTS; ++i) {
    uint8_t guessChar = guess.characters[i];

    if (target.characters[i] == guessChar) {
      // Correct position
      if (remainingCount[guessChar] > 0) {
        fb.setColor(i, 4); // Correct position, one more
      } else {
        fb.setColor(i, 2); // Correct position, no more
      }
    } else {
      // Not in correct position
      if (remainingCount[guessChar] > 0) {
        fb.setColor(i, 3); // Different position, one more
        remainingCount[guessChar]--;
      } else if (remainingCount[guessChar] == 0 &&
                 target.characterCount[guessChar] > 0) {
        fb.setColor(i, 1); // Different position, no more (already used up)
      } else {
        fb.setColor(i, 0); // Not present
      }
    }
  }

  return fb;
}

// Check if a pattern is valid according to game rules
bool isValidPattern(const Pattern &pattern) {
  std::array<uint8_t, NUM_CHARACTER_TYPES> characterTypeCounts = {};
  bool dragonNotInLast = false;

  for (uint8_t i = 0; i < NUM_SLOTS; ++i) {
    uint8_t c = pattern.characters[i];
    CharacterType cType = getCharacterType(c);
    characterTypeCounts[cType]++;

    if (c == DRAGON && i != 4) {
      dragonNotInLast = true;
    }
  }

  const auto &characterCounts = pattern.characterCount;

  // Position-based constraints
  for (uint8_t i = 0; i < NUM_SLOTS; ++i) {
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

  // Count-based constraints
  // The Mage always turns a monster into a frog
  if (characterCounts[MAGE] > 0 && characterCounts[FROG] == 0) {
    return false;
  }
  // Bats always come in a pair
  if (characterCounts[BAT] % 2 != 0) {
    return false;
  }
  // Spiders always come in triplets
  if (characterCounts[SPIDER] % 3 != 0) {
    return false;
  }
  // Axe Orcs and Blade Orcs always appear together
  if ((characterCounts[AXE_ORC] > 0 || characterCounts[BLADE_ORC] > 0) &&
      characterCounts[AXE_ORC] != characterCounts[BLADE_ORC]) {
    return false;
  }
  // The necromancer always turns all heroes into zombies
  if (characterCounts[NECROMANCER] > 0 && characterTypeCounts[HERO] > 0) {
    return false;
  }
  // The troll always comes without treasure
  if (characterCounts[TROLL] > 0 && characterTypeCounts[TREASURE] > 0) {
    return false;
  }
  // The dragon always comes with no other monsters
  if (characterCounts[DRAGON] > 0 && characterTypeCounts[MONSTER] > 1) {
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
std::vector<Pattern> generateAllPossiblePatterns() {
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
      Pattern pattern(characters);

      std::array<uint8_t, NUM_CHARACTER_TYPES> characterTypeCounts = {};
      bool dragonNotInLast = false;
      for (uint8_t i = 0; i < NUM_SLOTS; ++i) {
        uint8_t c = pattern.characters[i];
        CharacterType cType = getCharacterType(c);
        characterTypeCounts[cType]++;

        if (c == DRAGON && i != 4) {
          dragonNotInLast = true;
        }
      }

      // Use precomputed characterCount instead of recounting
      const auto &characterCounts = pattern.characterCount;

      // The Mage (2) always turns a monster into a frog
      if (characterCounts[MAGE] > 0 && characterCounts[FROG] == 0) {
        return;
      }
      // Bats always come in a pair
      if (characterCounts[BAT] % 2 != 0) {
        return;
      }
      // Spiders always come in triplets
      if (characterCounts[SPIDER] % 3 != 0) {
        return;
      }
      // Axe Orcs and Blade Orcs always appear together
      if ((characterCounts[AXE_ORC] > 0 || characterCounts[BLADE_ORC] > 0) &&
          characterCounts[AXE_ORC] != characterCounts[BLADE_ORC]) {
        return;
      }
      // The necromancer always turns all heroes into zombies
      if (characterCounts[NECROMANCER] > 0 && characterCounts[HERO] > 0) {
        return;
      }
      // The troll always comes without treasure
      if (characterCounts[TROLL] > 0 && characterTypeCounts[TREASURE] > 0) {
        return;
      }
      // The dragon always comes with no other monsters
      if (characterCounts[DRAGON] > 0 && characterTypeCounts[MONSTER] > 1) {
        return;
      }
      // The dragon always comes with no coins
      if (characterCounts[DRAGON] > 0 && characterCounts[COINS] > 0) {
        return;
      }
      // Coins come as a pair or triple
      if (characterCounts[COINS] % 2 != 0 && characterCounts[COINS] % 3 != 0) {
        return;
      }
      // If there is a dragon not in the last position, then there is a relic
      if (dragonNotInLast && characterCounts[RELIC] == 0) {
        return;
      }

      patterns.push_back(pattern);
      return;
    }

    for (uint8_t c = 0; c < NUM_CHARACTERS; ++c) {
      CharacterType cType = getCharacterType(c);
      // Heroes can appear in spots 0 and 1
      if ((cType == HERO || c == ZOMBIE) && !(pos == 0 || pos == 1)) {
        continue;
      }
      // The Knight always faces a monster
      if (pos > 0 && characters[pos - 1] == KNIGHT &&
          !(cType == MONSTER || c == FROG)) {
        continue;
      }
      // The Archer cannot face a monster
      if (pos > 0 && characters[pos - 1] == ARCHER &&
          (cType == MONSTER || c == FROG)) {
        continue;
      }
      // The bandit can only appear in the first position
      if (c == BANDIT && pos != 0) {
        continue;
      }
      // The sorcerer can only appear in the last position
      if (c == SORCERER && pos != 4) {
        continue;
      }
      // The troll always faces a hero or NPC
      uint8_t lastCType = pos > 0 ? getCharacterType(characters[pos - 1]) : 0;
      uint8_t lastC = pos > 0 ? characters[pos - 1] : 0;
      if (pos > 0 && c == TROLL &&
          !(lastCType == HERO || lastCType == NPC || lastC == ZOMBIE)) {
        continue;
      }
      // The villager can only appear in spots 0 or 1
      if (c == VILLAGER && !(pos == 0 || pos == 1)) {
        continue;
      }
      // The king can only appear in spot 0
      if (c == KING && pos != 0) {
        continue;
      }
      // The relic can only appear in spot 4
      if (c == RELIC && pos != 4) {
        continue;
      }

      characters[pos] = static_cast<uint8_t>(c);
      generate(pos + 1);
    }
  };

  generate(0);

  return patterns;
}

// Dungleon-specific ENT solver implementation
class DungleonEntSolver
    : public Utils::AbstractEntSolver<Pattern, Feedback, Config, PatternGuess,
                                      Result> {
protected:
  bool matchesFeedback(const Pattern &candidate,
                       const Feedback &feedback) const override {
    return Dungleon::matchesFeedback(candidate, feedback);
  }

  Feedback generateFeedback(const Pattern &target,
                            const Pattern &guess) const override {
    return Dungleon::generateFeedback(target, guess);
  }

  PatternGuess createGuess(const Pattern &candidate, double ent,
                           double probability) const override {
    PatternGuess guess;
    guess.pattern = candidate;
    guess.ent = ent;
    guess.probability = probability;
    return guess;
  }

  Result createResult(const std::vector<PatternGuess> &guesses,
                      int totalPossible) const override {
    Result result;
    result.sortedGuesses = guesses;
    result.totalPossiblePatterns = totalPossible;
    return result;
  }
};

Result runDungleonSolver(const Config &config, std::atomic<bool> *cancel) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif

  std::vector<Pattern> allPatterns = generateAllPossiblePatterns();
  std::vector<Pattern> possiblePatterns = generateAllPossiblePatterns();

  // Use the specialized Dungleon ENT solver - returns Result directly!
  DungleonEntSolver solver;
  return solver.solve(allPatterns, possiblePatterns, config, cancel);
}
} // namespace Dungleon