#pragma once
#include <array>
#include <string>
#include <vector>

#include "utils/inputUtils.hpp"
#include "utils/wordUtils.hpp"
#include <atomic>

namespace SpellingBee {
struct Config {
  std::array<char, 7> allLetters;
  std::array<bool, 256> validLettersMap = {false};
  bool excludeUncommonWords = false;
};

std::vector<Utils::Word>
runSpellingBeeSolver(const Config &config, std::atomic<bool> *cancel = nullptr);
} // namespace SpellingBee
