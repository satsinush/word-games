#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iostream>
#include <map>
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
Feedback parseFeedback(const std::string &input) {
  Feedback fb;
  // TODO
  return fb;
}

// Helper: Check if a pattern matches all feedback constraints
bool matchesFeedback(const Pattern &candidate, const Feedback &fb) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  const Pattern &guess = fb.pattern;

  // TODO: optimize this by precomuting character counts in Pattern
  std::array<uint8_t, NUM_CHARACTERS> candidateCount = {};
  for (uint8_t i = 0; i < NUM_SLOTS; ++i) {
    uint8_t character = candidate.characters[i];
    candidateCount[character]++;
  }

  // Check greens first, and adjust counts
  for (size_t i = 0; i < NUM_SLOTS; ++i) {
    if (fb.colors[i * 2 + 1]) // Green
    {                         // Green
      if (candidate.characters[i] != guess.characters[i]) {
        return false; // Must match green letters
      }
      candidateCount[candidate.characters[i]]--;
    }
  }

  for (size_t i = 0; i < NUM_SLOTS; ++i) {
    uint8_t guessCharacter = guess.characters[i];
    if (fb.colors[i * 2 + 1]) { // Green (already checked)
      continue;
    } else if (fb.colors[i * 2]) { // Yellow
      if (candidate.characters[i] == guessCharacter) {
        return false; // Yellow letter cannot be in the same spot
      }
      if (candidateCount[guessCharacter] <= 0) {
        return false; // Must contain the yellow letter elsewhere
      }
      candidateCount[guessCharacter]--;
    } else { // Grey
      if (candidateCount[guessCharacter] > 0) {
        return false; // Candidate has a letter that was marked grey
      }
    }
  }

  return true;
}

// Generate feedback for a guess against a target word
Feedback generateFeedback(const Pattern &target, const Pattern &guess) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  Feedback fb;
  fb.pattern = guess;

  // TODO: optimize this by precomuting character counts in Pattern
  std::array<uint8_t, NUM_CHARACTERS> candidateCount = {};
  for (uint8_t i = 0; i < NUM_SLOTS; ++i) {
    uint8_t character = target.characters[i];
    candidateCount[character]++;
  }

  // First pass: Mark greens. A green is represented by setting both bits for a
  // position to 1. Example for position i: bit (i*2) = 1, bit (i*2 + 1) = 1.
  for (size_t i = 0; i < NUM_SLOTS; ++i) {
    if (target.characters[i] == guess.characters[i]) {
      // Direct manipulation: Set the 'green' bit and the 'yellow' bit.
      fb.colors[i * 2 + 1] = 1; // This bit signals "correct position" (Green).
      fb.colors[i * 2] = 1;     // This bit signals "in word" (Green or Yellow).

      candidateCount[target.characters[i]]--;
    }
  }

  // Second pass: Mark yellows. A yellow is when the "in word" bit is 1 but
  // "correct position" is 0.
  for (size_t i = 0; i < NUM_SLOTS; ++i) {
    if (!fb.colors[i * 2 + 1]) // If it's NOT green...
    {
      int guessCharIndex = guess.characters[i];
      if (candidateCount[guessCharIndex] > 0) {
        fb.colors[i * 2] = 1;
        candidateCount[guessCharIndex]--;
      }
    }
  }

  return fb;
}

// Generate all possible patterns for the given configuration
std::vector<Pattern> generateAllPatterns() {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif
  std::vector<Pattern> patterns;

  // Generate all possible combinations with repetition
  Pattern current;

  std::function<void(unsigned int)> generate = [&](unsigned int pos) {
    if (pos == NUM_SLOTS) {
      patterns.push_back(current);
      return;
    }

    for (unsigned int c = 0; c < NUM_CHARACTERS; ++c) {
      current.characters[pos] = static_cast<uint8_t>(c);
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
  Pattern current;

  std::function<void(unsigned int)> generate = [&](unsigned int pos) {
    if (pos == NUM_SLOTS) {
      std::array<uint8_t, NUM_CHARACTERS> characterCounts = {};
      std::array<uint8_t, NUM_CHARACTER_TYPES> characterTypeCounts = {};
      bool dragonNotInLast = false;
      for (uint8_t i = 0; i < NUM_SLOTS; ++i) {
        uint8_t c = current.characters[i];
        CharacterType cType = getCharacterType(c);
        characterCounts[c]++;
        characterTypeCounts[cType]++;

        if (c == DRAGON && i != 4) {
          dragonNotInLast = true;
        }
      }

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

      patterns.push_back(current);
      return;
    }

    for (unsigned int c = 0; c < NUM_CHARACTERS; ++c) {
      CharacterType cType = getCharacterType(c);
      // Heroes can appear in spots 0 and 1
      if ((cType == HERO || c == ZOMBIE) && !(pos == 0 || pos == 1)) {
        return;
      }
      // The Knight always faces a monster
      if (pos > 0 && current.characters[pos - 1] == KNIGHT &&
          !(cType == MONSTER || c == FROG)) {
        return;
      }
      // The Archer cannot face a monster
      if (pos > 0 && current.characters[pos - 1] == ARCHER &&
          (cType == MONSTER || c == FROG)) {
        return;
      }
      // The bandit can only appear in the first position
      if (c == BANDIT && pos != 0) {
        return;
      }
      // The sorcerer can only appear in the last position
      if (c == SORCERER && pos != 4) {
        return;
      }
      // The troll always faces a hero or NPC
      // TODO: fix this so that it checks pos - 1 as pos + 1 will always be 0
      if (pos < 4 && current.characters[pos + 1] == TROLL &&
          !(cType == HERO || cType == NPC || c == ZOMBIE)) {
        return;
      }
      // The villager can only appear in spots 0 or 1
      if (c == VILLAGER && !(pos == 0 || pos == 1)) {
        return;
      }
      // The king can only appear in spot 0
      if (c == KING && pos != 0) {
        return;
      }
      // The relic can only appear in spot 4
      if (c == RELIC && pos != 4) {
        return;
      }

      current.characters[pos] = static_cast<uint8_t>(c);
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

Result runDungleonSolver(const std::vector<Pattern> &allPatterns,
                         const std::vector<Feedback> &feedbacks,
                         const Config &config) {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif

  // Use the specialized Dungleon ENT solver - returns Result directly!
  DungleonEntSolver solver;
  return solver.solve(allPatterns, feedbacks, config);
}
} // namespace Dungleon