#pragma once
#include <array>
#include <atomic>
#include <bitset>
#include <cmath>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "utils/EntSolver.hpp"
#include "utils/inputUtils.hpp"
#include "utils/utils.hpp"

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

// Declare global variables and functions as extern to avoid multiple
// definitions
extern std::array<std::string, NUM_CHARACTERS> CHARACTER_IDS;
extern std::array<std::string, NUM_CHARACTERS> CHARACTER_NAMES;
CharacterType getCharacterType(uint8_t characterId);

struct Feedback; // forward declaration so Config can reference Feedback

struct Pattern; // forward declaration so Config can reference Pattern

struct Config {
  uint8_t maxDepth = 1; // How many moves ahead to calculate ENT
  bool autoDepth = false; // Dynamically choose optimal depth
  bool excludeImpossiblePatterns =
      false; // Whether to exclude impossible patterns from guesses
  uint32_t maxGuesses = 10; // Maximum allowed guesses
  std::vector<Feedback> feedbackHistory = {}; // History of previous feedbacks
  std::vector<Pattern> solutionHistory =
      {}; // History of previous solutions, used for Gauntlet mode where each
          // solution must share at least one character with all the previous
          // ones
  std::array<bool, NUM_CHARACTERS> sharedCharacters;
};

struct Pattern {
  std::array<uint8_t, 5> characters; // Array of character values
  double score = 1.0;                // Score for weighting (default 1.0)
  std::array<uint8_t, NUM_CHARACTERS> characterCount =
      {}; // Precomputed character occurrence counts

  Pattern() { characters.fill(0); }
  Pattern(const std::array<uint8_t, 5> &c) : characters(c) {
    computeCharacterCount();
  }

  // Compute character occurrence counts
  void computeCharacterCount() {
    characterCount.fill(0);
    for (uint8_t i = 0; i < NUM_SLOTS; ++i) {
      characterCount[characters[i]]++;
    }
  }

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
  std::bitset<15>
      colors; // 3 bits per position (5 positions * 3 bits = 15 bits)

  bool operator==(const Feedback &other) const {
    return pattern == other.pattern && colors == other.colors;
  }

  bool operator<(const Feedback &other) const {
    if (pattern != other.pattern)
      return pattern < other.pattern;
    return colors.to_ulong() < other.colors.to_ulong();
  }

  // Helper methods to get/set feedback for position i
  // 0 = not present
  // 1 = different position, no more
  // 2 = correct position, no more
  // 3 = different position, one more
  // 4 = correct position, one more
  void setColor(const int i, const int color) {
    int bitPos = i * 3;
    colors[bitPos] = (color & 1) != 0;
    colors[bitPos + 1] = (color & 2) != 0;
    colors[bitPos + 2] = (color & 4) != 0;
  }

  int getColor(const int i) const {
    int bitPos = i * 3;
    int color = 0;
    if (colors[bitPos])
      color |= 1;
    if (colors[bitPos + 1])
      color |= 2;
    if (colors[bitPos + 2])
      color |= 4;
    return color;
  }
};

struct PatternGuess {
  Pattern pattern;
  double ent = 0.0; // Expected Number of Turns
  double wnt = 0.0; // Worst Number of Turns
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
  
    // Third tiebreaker: WNT (lower is better)
    if (std::abs(wnt - other.wnt) > tolerance)
      return wnt < other.wnt;

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

// Check if a pattern is valid according to game rules
bool isValidPattern(const Pattern &pattern, const Config &config,
                    uint8_t numSlots = NUM_SLOTS);

// Generate feedback for a guess against a target pattern
Feedback generateFeedback(const Pattern &target, const Pattern &guess);

// Generate all patterns for the given configuration
std::vector<Pattern> generateAllPatterns();

// Generate all possible patterns for the given configuration
std::vector<Pattern> generateAllPossiblePatterns(const Config &config);

// Generic EntSolver-based version (cleaner implementation)
Result runDungleonSolver(const Config &config = Config{},
                         std::atomic<bool> *cancel = nullptr);
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
