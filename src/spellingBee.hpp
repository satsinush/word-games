#pragma once
#include <string>
#include <vector>
#include <array>

#include "utils.hpp"

namespace SpellingBee
{
    struct Config
    {
        std::array<char, 7> allLetters;
        std::array<bool, 256> validLettersMap = {false};
    };

    std::vector<WordUtils::Word> runSpellingBeeSolver(const std::vector<WordUtils::Word> &words, const Config &config);
}
