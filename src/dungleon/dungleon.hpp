#pragma once
#include <array>
#include <bitset>
#include <cmath>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "utils/EntSolver.hpp"
#include "utils/inputUtils.hpp"
#include "utils/wordUtils.hpp"

namespace Dungleon {

constexpr uint8_t NUM_SLOTS = 5;
constexpr uint8_t NUM_CHARACTERS = 20;
constexpr uint8_t NUM_CHARACTER_TYPES = 5;

enum CharacterType { HERO = 0, MONSTER = 1, NPC = 2, TREASURE = 3, OTHER = 4 };

enum Character {
  // HERO
  ARCHER = 0,
  KNIGHT = 1,
  MAGE = 2,
  // MONSTER
  BAT = 3,
  DRAGON = 4,
  BLADE_ORC = 5,
  NECROMANCER = 6,
  AXE_ORC = 7,
  SKELETON = 8,
  SPIDER = 9,
  BANDIT = 10,
  TROLL = 11,
  SORCERER = 12,
  // NPC
  KING = 13,
  VILLAGER = 14,
  // TREASURE
  COINS = 15,
  CHEST = 16,
  RELIC = 17,
  // OTHER
  FROG = 18,
  ZOMBIE = 19
};

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

std::array<std::string, NUM_CHARACTERS> CHARACTER_IDS = {
    "ar", "kn", "ma", "bt", "dr", "bo", "ne", "ao", "sk", "sp",
    "bd", "tr", "so", "ki", "vi", "co", "ch", "re", "fr", "zo"};

std::array<std::string, NUM_CHARACTERS> CHARACTER_NAMES = {
    "archer",    "knight",      "mage",     "bat",      "dragon",
    "blade orc", "necromancer", "axe orc",  "skeleton", "spider",
    "bandit",    "troll",       "sorcerer", "king",     "villager",
    "coins",     "chest",       "relic",    "frog",     "zombie"};

struct Config {
  uint8_t maxDepth = 0; // How many moves ahead to calculate ENT
};

struct Pattern {
  std::array<uint8_t, 5> characters; // Array of character values
  double score = 1.0;                // Score for weighting (default 1.0)

  Pattern() { characters.fill(0); }
  Pattern(const std::array<uint8_t, 5> &c) : characters(c) {}

  bool operator<(const Pattern &other) const {
    for (uint8_t i = 0; i < 5; ++i) {
      if (characters[i] != other.characters[i])
        return characters[i] < other.characters[i];
    }
    return false;
  }

  bool operator==(const Pattern &other) const {
    for (size_t i = 0; i < 5; ++i) {
      if (characters[i] != other.characters[i])
        return false;
    }
    return true;
  }

  std::string toString() const {
    std::string result;
    for (uint8_t i = 0; i < 5; ++i) {
      if (i > 0)
        result += " ";
      result += CHARACTER_IDS[characters[i]];
    }
    return result;
  }
};

struct Feedback {
  Pattern pattern;
  std::bitset<10> colors;

  bool operator==(const Feedback &other) const {
    return pattern == other.pattern && colors == other.colors;
  }

  bool operator<(const Feedback &other) const {
    if (pattern != other.pattern)
      return pattern < other.pattern;
    return colors.to_ulong() < other.colors.to_ulong();
  }

  // Helper methods to get/set feedback for position i
  void setGrey(const int i) {
    colors.reset(i * 2);
    colors.reset(i * 2 + 1);
  }

  void setYellow(const int i) {
    colors.set(i * 2);
    colors.reset(i * 2 + 1);
  }

  void setGreen(const int i) {
    colors.set(i * 2);
    colors.set(i * 2 + 1);
  }

  int getColor(const int i) const {
    if (colors[i * 2 + 1])
      return 2; // green
    if (colors[i * 2])
      return 1; // yellow
    return 0;   // grey
  }
};

struct PatternGuess {
  Pattern pattern;
  double ent = 0.0; // Expected Number of Turns
  double probability = 0.0;

  bool operator<(const PatternGuess &other) const {
    // Primary sort: Expected Number of Turns (lower is better)
    const double tolerance = 1e-9;

    if (std::abs(ent - other.ent) > tolerance)
      return ent < other.ent; // Sort lower ENT first (fewer expected turns)

    // Secondary tiebreaker: probability (higher is better - prefer possible
    // answers)
    if (std::abs(probability - other.probability) > tolerance)
      return probability > other.probability; // Sort higher probability first

    // Final tiebreaker: sort by pattern for consistency
    return pattern < other.pattern;
  }
};

struct Result {
  std::vector<PatternGuess> sortedGuesses;
  int totalPossiblePatterns = 0;
};

// Parse feedback string like "rgbc 2 1" (pattern correctPos correctCol)
Feedback parseFeedback(const std::string &input, const Config &config);

// Check if a pattern matches feedback constraints
bool matchesFeedback(const Pattern &candidate, const Feedback &fb);

// Generate feedback for a guess against a target pattern
Feedback generateFeedback(const Pattern &target, const Pattern &guess);

// Generate all possible patterns for the given configuration
std::vector<Pattern> generateAllPatterns();

// Generic EntSolver-based version (cleaner implementation)
Result runDungleonSolver(const std::vector<Pattern> &allPatterns,
                         const std::vector<Feedback> &guessHistory,
                         const Config &config = Config{});
} // namespace Dungleon

// Provide std::hash specializations for Mastermind types so they can be used as
// unordered_map/unordered_set keys in generic code (like the EntSolver).
namespace std {
template <> struct hash<Dungleon::Pattern> {
  size_t operator()(const Dungleon::Pattern &pattern) const noexcept {
    // Hash the characters array using FNV-1a style mixing
    size_t hash = 2166136261u;
    for (const auto &c : pattern.characters) {
      hash ^= c;
      hash *= 16777619;
    }
    return hash;
  }
};

template <> struct hash<Dungleon::Feedback> {
  size_t operator()(const Dungleon::Feedback &fb) const noexcept {
    // Combine hash of the pattern and the bitset colors
    size_t hash = std::hash<Dungleon::Pattern>()(fb.pattern);
    hash ^= fb.colors.to_ulong() + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
  }
};
} // namespace std
