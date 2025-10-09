#pragma once
#include <string>
#include <vector>
#include <array>

#include "../utils/utils.hpp"

namespace SpellingBee
{
    struct Config
    {
        std::array<char, 7> allLetters;
        std::array<bool, 256> validLettersMap = {false};
    };

    std::vector<Utils::Word> runSpellingBeeSolver(const std::vector<Utils::Word> &words, const Config &config);
}
