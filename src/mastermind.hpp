#pragma once
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <cmath>

#include "utils.hpp"

namespace Mastermind
{
    struct Config
    {
        int numPegs = 4;             // Number of pegs in the pattern
        int numColors = 6;           // Total number of available colors (0 to numColors-1)
        bool allowDuplicates = true; // Whether duplicate colors are allowed
        int maxDepth = 0;            // How many moves ahead to calculate entropy
    };

    struct Pattern
    {
        std::vector<uint8_t> colors; // Array of color values
        int order = 0;               // For compatibility with Word structure
        int count = 1;               // For compatibility with Word structure

        Pattern() = default;
        Pattern(const std::vector<uint8_t> &c) : colors(c) {}
        Pattern(int numPegs) : colors(numPegs, 0) {}

        bool operator<(const Pattern &other) const
        {
            return colors < other.colors;
        }

        bool operator==(const Pattern &other) const
        {
            return colors == other.colors;
        }

        std::string toString() const
        {
            std::string result;
            for (size_t i = 0; i < colors.size(); ++i)
            {
                if (i > 0)
                    result += " ";
                result += std::to_string(colors[i]);
            }
            return result;
        }
    };

    struct Feedback
    {
        Pattern guess;               // The guessed pattern
        uint8_t correctPosition = 0; // Number of pegs in correct position (like green in wordle)
        uint8_t correctColor = 0;    // Number of colors correct but wrong position (like yellow in wordle)

        bool operator==(const Feedback &other) const
        {
            return guess.colors == other.guess.colors &&
                   correctPosition == other.correctPosition &&
                   correctColor == other.correctColor;
        }

        bool operator<(const Feedback &other) const
        {
            if (guess.colors != other.guess.colors)
                return guess.colors < other.guess.colors;
            if (correctPosition != other.correctPosition)
                return correctPosition < other.correctPosition;
            return correctColor < other.correctColor;
        }
    };

    struct PatternGuess
    {
        Pattern pattern;
        double entropy = 0.0;
        double probability = 0.0;
        std::vector<double> entropyList;

        bool operator<(const PatternGuess &other) const
        {
            // Compare entropy levels from highest depth to lowest (E3, E2, E1)
            // We want higher entropy values to come first (better guesses)
            int maxLevels = std::max(entropyList.size(), other.entropyList.size());

            // Use small tolerance for floating point comparison
            const double tolerance = 1e-9;

            for (int i = maxLevels - 1; i >= 0; i--)
            {
                double thisEntropy = (i < entropyList.size()) ? entropyList[i] : 0.0;
                double otherEntropy = (i < other.entropyList.size()) ? other.entropyList[i] : 0.0;

                if (std::abs(thisEntropy - otherEntropy) > tolerance)
                    return thisEntropy > otherEntropy; // Sort in descending order (higher entropy first)
            }

            // If all entropy levels are equal, prefer higher probability
            if (std::abs(probability - other.probability) > tolerance)
                return probability > other.probability; // Sort in descending order (higher probability first)

            // If both entropy and probability are equal, sort by pattern for consistency
            return pattern.colors < other.pattern.colors;
        }
    };

    struct Result
    {
        std::vector<PatternGuess> sortedGuesses;
        int totalPossiblePatterns = 0;
    };

    // Parse feedback string like "2 1" (2 correct position, 1 correct color)
    Feedback parseFeedback(const std::string &input, int numPegs);

    // Check if a pattern matches feedback constraints
    bool matchesFeedback(const Pattern &candidate, const Feedback &fb);

    // Generate feedback for a guess against a target pattern
    Feedback generateFeedback(const Pattern &target, const Pattern &guess);

    // Calculate entropy (information value) of a guess
    double calculateEntropy(const std::vector<Pattern> &possiblePatterns,
                            const Pattern &guess,
                            const Config &config);

    // Generate all possible patterns for the given configuration
    std::vector<Pattern> generateAllPatterns(const Config &config);

    // Filter possible patterns given a list of guesses and feedbacks
    std::vector<Pattern> filterPatterns(
        const std::vector<Pattern> &patterns,
        const std::vector<Feedback> &guessHistory);

    // Calculate best guesses sorted by information value with multi-depth entropy
    std::vector<PatternGuess> calculateBestGuesses(
        const std::vector<Pattern> &allPatterns,
        const std::vector<Pattern> &possiblePatterns,
        const std::vector<Feedback> &guessHistory,
        const Config &config = Config{},
        int recursionLevel = 0);

    // Enhanced solver that returns best guesses ranked by entropy and possible pattern count
    Result runMastermindSolverWithEntropy(
        const std::vector<Pattern> &allPatterns,
        const std::vector<Feedback> &guessHistory,
        const Config &config = Config{});
}
