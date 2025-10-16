#pragma once
#include <array>
#include <string>
#include <vector>

#include "utils/inputUtils.hpp"
#include "utils/profilerUtils.hpp"
#include "utils/wordUtils.hpp"

namespace SpellingBee {
struct Config {
  std::array<char, 7> allLetters;
  std::array<bool, 256> validLettersMap = {false};
};

std::vector<Utils::Word>
runSpellingBeeSolver(const std::vector<Utils::Word> &words,
                     const Config &config);
} // namespace SpellingBee
