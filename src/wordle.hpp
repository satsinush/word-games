#pragma once
#include <string>
#include <vector>
#include <array>
#include <bitset>

#include "utils.hpp"

namespace Wordle
{
    struct Config
    {
        int maxDepth = 1; // How many moves ahead to calculate entropy
    };

    struct Feedback
    {
        std::string word;       // 5-letter guess
        std::bitset<10> colors; // Optimized: bits 0,2,4,6,8 = letter in word, bits 1,3,5,7,9 = correct position

        bool operator==(const Feedback &other) const
        {
            return word == other.word && colors == other.colors;
        }

        bool operator<(const Feedback &other) const
        {
            if (word != other.word)
                return word < other.word;
            return colors.to_ulong() < other.colors.to_ulong();
        }

        // Helper methods to get/set feedback for position i
        void setGrey(int i)
        {
            colors.reset(i * 2);
            colors.reset(i * 2 + 1);
        }

        void setYellow(int i)
        {
            colors.set(i * 2);
            colors.reset(i * 2 + 1);
        }

        void setGreen(int i)
        {
            colors.set(i * 2);
            colors.set(i * 2 + 1);
        }

        int getColor(int i) const
        {
            if (colors[i * 2 + 1])
                return 2; // green
            if (colors[i * 2])
                return 1; // yellow
            return 0;     // grey
        }
    };

    struct WordGuess
    {
        WordUtils::Word word;
        double entropy = 0.0;
        double probability = 0.0;
        std::vector<double> entropyList;

        bool operator<(const WordGuess &other) const
        {
            // Sort by entropy (higher is better), then by probability (higher is better)
            if (entropy != other.entropy)
                return entropy > other.entropy;
            return probability > other.probability;
        }
    };

    struct Result
    {
        std::vector<WordGuess> sortedGuesses;
        int totalPossibleWords = 0;
    };

    // Parse feedback string like "STEAL 01201"
    Feedback parseFeedback(const std::string &input);

    // Check if a word matches feedback constraints
    bool matchesFeedback(const WordUtils::Word &candidate, const Feedback &fb);

    // Generate feedback for a guess against a target word
    Feedback generateFeedback(const WordUtils::Word &target, const std::string &guess);

    // Get all possible feedback patterns for a 5-letter word
    std::vector<Feedback> getAllPossibleFeedbacks();

    // Calculate entropy (information value) of a guess
    double calculateEntropy(const std::vector<WordUtils::Word> &possibleWords,
                            const WordUtils::Word &guess);

    // Filter possible words given a list of guesses and feedbacks
    std::vector<WordUtils::Word> filterWords(
        const std::vector<WordUtils::Word> &words,
        const std::vector<Feedback> &feedbacks);

    // Calculate best guesses sorted by information value with multi-depth entropy
    std::vector<WordGuess> calculateBestGuesses(
        const std::vector<WordUtils::Word> &allWords,
        const std::vector<WordUtils::Word> &possibleWords,
        const std::vector<Feedback> &feedbackHistory,
        const Config &config = Config{},
        int recursionLevel = 0);

    // Enhanced solver that returns best guesses ranked by entropy and possible word count
    // If config.maxDepth is 0, skips entropy calculation and just returns filtered words
    Result runWordleSolverWithEntropy(
        const std::vector<WordUtils::Word> &allWords,
        const std::vector<Feedback> &feedbacks,
        const Config &config = Config{});
}