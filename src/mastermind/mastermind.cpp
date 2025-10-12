#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <iostream>
#include <cctype>
#include <cmath>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <sstream>
#include <sstream>

#include "mastermind.hpp"
#include "../utils/EntropySolver.hpp"

namespace Mastermind
{
    Feedback parseFeedback(const std::string &input, unsigned int numPegs)
    {
        // Split input by pipe separator
        size_t pipePos = input.find('|');
        if (pipePos == std::string::npos)
        {
            throw std::runtime_error("Invalid format. Expected: 'pattern|feedback' (e.g., '1 2 3 4|2 1')");
        }

        std::string patternStr = input.substr(0, pipePos);
        std::string feedbackStr = input.substr(pipePos + 1);

        // Parse guess pattern
        std::istringstream patternIss(patternStr);
        Pattern guess;
        guess.numPegs = 0;
        std::string token;
        while (patternIss >> token && guess.numPegs < MAX_PEGS)
        {
            if (token.length() != 1 || !std::isdigit(token[0]))
            {
                throw std::runtime_error("Pattern must contain only single digit numbers");
            }
            guess.colors[guess.numPegs] = token[0] - '0';
            guess.numPegs++;
        }

        if (guess.numPegs != numPegs)
        {
            throw std::runtime_error("Pattern must have exactly " + std::to_string(numPegs) + " colors");
        }

        // Parse feedback
        std::istringstream feedbackIss(feedbackStr);
        int correctPos, correctCol;
        if (!(feedbackIss >> correctPos >> correctCol))
        {
            throw std::runtime_error("Invalid feedback format. Expected: 'correctPosition correctColor' (e.g., '2 1')");
        }

        if (correctPos < 0 || correctPos > static_cast<int>(numPegs) || correctCol < 0 || correctCol > static_cast<int>(numPegs))
        {
            throw std::runtime_error("Feedback values must be between 0 and number of pegs");
        }

        Feedback fb;
        fb.guess = guess;
        fb.correctPosition = static_cast<uint8_t>(correctPos);
        fb.correctColor = static_cast<uint8_t>(correctCol);
        return fb;
    }

    // Helper: Check if a pattern matches feedback constraints
    bool matchesFeedback(const Pattern &candidate, const Feedback &fb)
    {
        const Pattern &guess = fb.guess;
        if (candidate.numPegs != guess.numPegs)
            return false;

        // Count color occurrences in candidate using vector for better cache performance
        // uint8_t can only have values 0-255, so reserve 256 spots
        std::array<int, 256> candidateCount = {};
        for (uint8_t color : candidate.colors)
        {
            candidateCount[color]++;
        }

        // Count correct positions and adjust counts
        int correctPositions = 0;
        for (size_t i = 0; i < candidate.numPegs; ++i)
        {
            if (candidate.colors[i] == guess.colors[i])
            {
                correctPositions++;
                candidateCount[candidate.colors[i]]--;
            }
        }

        if (correctPositions != fb.correctPosition)
            return false;

        // Count correct colors in wrong positions
        int correctColors = 0;
        for (size_t i = 0; i < guess.numPegs; ++i)
        {
            // Skip positions that were already correct
            if (candidate.colors[i] == guess.colors[i])
                continue;

            if (candidateCount[guess.colors[i]] > 0)
            {
                correctColors++;
                candidateCount[guess.colors[i]]--;
            }
        }

        return correctColors == fb.correctColor;
    }

    // Generate feedback for a guess against a target pattern
    Feedback generateFeedback(const Pattern &target, const Pattern &guess)
    {
        Feedback fb;
        fb.guess = guess; // Store the guess in the feedback

        if (target.numPegs != guess.numPegs)
            return fb; // Invalid input

        // Count color occurrences in target using vector for better cache performance
        // uint8_t can only have values 0-255, so reserve 256 spots
        std::array<int, 256> targetCount = {};
        for (size_t i = 0; i < target.numPegs; ++i)
        {
            targetCount[target.colors[i]]++;
        }

        // First pass: count correct positions
        for (size_t i = 0; i < target.numPegs; ++i)
        {
            if (target.colors[i] == guess.colors[i])
            {
                fb.correctPosition++;
                targetCount[target.colors[i]]--;
            }
        }

        // Second pass: count correct colors in wrong positions
        for (size_t i = 0; i < guess.numPegs; ++i)
        {
            // Skip positions that were already correct
            if (target.colors[i] == guess.colors[i])
                continue;

            if (targetCount[guess.colors[i]] > 0)
            {
                fb.correctColor++;
                targetCount[guess.colors[i]]--;
            }
        }

        return fb;
    }

    // Calculate information bits
    double bits(double probability)
    {
        if (probability <= 0.0)
            return 0.0;
        return -std::log2(probability);
    }

    // Calculate entropy (information value) of a guess
    double calculateEntropy(const std::vector<Pattern> &possiblePatterns,
                            const Pattern &guess)
    {
        if (possiblePatterns.empty())
            return 0.0;

        std::map<Feedback, int> feedbackCounts;

        // For each possible target pattern, generate feedback and count
        for (const auto &target : possiblePatterns)
        {
            Feedback fb = generateFeedback(target, guess);
            feedbackCounts[fb]++;
        }

        // Calculate entropy
        double entropy = 0.0;
        int totalPatterns = possiblePatterns.size();

        for (const auto &pair : feedbackCounts)
        {
            double probability = static_cast<double>(pair.second) / totalPatterns;
            entropy += probability * bits(probability);
        }

        return entropy;
    }

    // Generate all possible patterns for the given configuration
    std::vector<Pattern> generateAllPatterns(const Config &config)
    {
        std::vector<Pattern> patterns;

        if (config.allowDuplicates)
        {
            // Generate all possible combinations with repetition
            Pattern current;
            current.numPegs = config.numPegs;

            std::function<void(unsigned int)> generate = [&](unsigned int pos)
            {
                if (pos == config.numPegs)
                {
                    patterns.push_back(current);
                    return;
                }

                for (unsigned int color = 0; color < config.numColors; ++color)
                {
                    current.colors[pos] = static_cast<uint8_t>(color);
                    generate(pos + 1);
                }
            };

            generate(0);
        }
        else
        {
            // Generate all possible permutations without repetition
            if (config.numColors < config.numPegs)
            {
                // Not enough colors for the number of pegs
                return patterns;
            }

            std::array<uint8_t, 256> availableColors; // Support up to 256 colors
            for (unsigned int i = 0; i < config.numColors; ++i)
            {
                availableColors[i] = static_cast<uint8_t>(i);
            }

            Pattern current;
            current.numPegs = config.numPegs;
            std::vector<bool> used(config.numColors, false);

            std::function<void(unsigned int)> generate = [&](unsigned int pos)
            {
                if (pos == config.numPegs)
                {
                    patterns.push_back(current);
                    return;
                }

                for (unsigned int i = 0; i < config.numColors; ++i)
                {
                    if (!used[i])
                    {
                        used[i] = true;
                        current.colors[pos] = static_cast<uint8_t>(i);
                        generate(pos + 1);
                        used[i] = false;
                    }
                }
            };

            generate(0);
        }

        return patterns;
    }

    std::vector<Pattern> filterPatterns(
        const std::vector<Pattern> &patterns,
        const std::vector<Feedback> &guessHistory)
    {
        std::vector<Pattern> filtered;
        for (const auto &pattern : patterns)
        {
            bool matches = true;
            for (const Feedback &feedback : guessHistory)
            {
                if (!matchesFeedback(pattern, feedback))
                {
                    matches = false;
                    break;
                }
            }
            if (matches)
                filtered.push_back(pattern);
        }
        return filtered;
    }

    // Mastermind-specific entropy solver implementation
    class MastermindEntropySolver : public Utils::AbstractEntropySolver<Pattern, Feedback, Config, PatternGuess, Result>
    {
    protected:
        bool matchesFeedback(const Pattern &candidate, const Feedback &feedback) const override
        {
            return Mastermind::matchesFeedback(candidate, feedback);
        }

        Feedback generateFeedback(const Pattern &target, const Pattern &guess) const override
        {
            return Mastermind::generateFeedback(target, guess);
        }

        PatternGuess createGuess(const Pattern &candidate, double entropy, double probability, const std::vector<double> &entropyList) const override
        {
            PatternGuess guess;
            guess.pattern = candidate;
            guess.entropy = entropy;
            guess.probability = probability;
            guess.entropyList = entropyList;
            return guess;
        }

        Result createResult(const std::vector<PatternGuess> &guesses, int totalPossible) const override
        {
            Result result;
            result.sortedGuesses = guesses;
            result.totalPossiblePatterns = totalPossible;
            return result;
        }
    };

    Result runMastermindSolver(
        const std::vector<Pattern> &allPatterns,
        const std::vector<Feedback> &guessHistory,
        const Config &config)
    {

        // Use the specialized Mastermind entropy solver
        MastermindEntropySolver solver;
        // Use the specialized Mastermind entropy solver - returns Result directly!
        return solver.solve(allPatterns, guessHistory, config);
    }
}
