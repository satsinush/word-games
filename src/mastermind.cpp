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
#include "utils.hpp"
#include "mastermind.hpp"

namespace Mastermind
{
    Feedback parseFeedback(const std::string &input, int numPegs)
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
        guess.colors.clear();
        std::string token;
        while (patternIss >> token)
        {
            if (token.length() != 1 || !std::isdigit(token[0]))
            {
                throw std::runtime_error("Pattern must contain only single digit numbers");
            }
            guess.colors.push_back(token[0] - '0');
        }

        if (guess.colors.size() != numPegs)
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

        if (correctPos < 0 || correctPos > numPegs || correctCol < 0 || correctCol > numPegs)
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
        if (candidate.colors.size() != guess.colors.size())
            return false;

        // Count color occurrences in candidate
        std::map<uint8_t, int> candidateCount;
        for (uint8_t color : candidate.colors)
        {
            candidateCount[color]++;
        }

        // Count correct positions and adjust counts
        int correctPositions = 0;
        for (size_t i = 0; i < candidate.colors.size(); ++i)
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
        for (size_t i = 0; i < guess.colors.size(); ++i)
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

        if (target.colors.size() != guess.colors.size())
            return fb; // Invalid input

        // Count color occurrences in target
        std::map<uint8_t, int> targetCount;
        for (uint8_t color : target.colors)
        {
            targetCount[color]++;
        }

        // First pass: count correct positions
        for (size_t i = 0; i < target.colors.size(); ++i)
        {
            if (target.colors[i] == guess.colors[i])
            {
                fb.correctPosition++;
                targetCount[target.colors[i]]--;
            }
        }

        // Second pass: count correct colors in wrong positions
        for (size_t i = 0; i < guess.colors.size(); ++i)
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
                            const Pattern &guess,
                            const Config &config)
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
            std::vector<uint8_t> current(config.numPegs, 0);

            std::function<void(int)> generate = [&](int pos)
            {
                if (pos == config.numPegs)
                {
                    patterns.emplace_back(current);
                    return;
                }

                for (int color = 0; color < config.numColors; ++color)
                {
                    current[pos] = static_cast<uint8_t>(color);
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

            std::vector<uint8_t> colors(config.numColors);
            for (int i = 0; i < config.numColors; ++i)
            {
                colors[i] = static_cast<uint8_t>(i);
            }

            std::vector<uint8_t> current(config.numPegs);
            std::vector<bool> used(config.numColors, false);

            std::function<void(int)> generate = [&](int pos)
            {
                if (pos == config.numPegs)
                {
                    patterns.emplace_back(current);
                    return;
                }

                for (int i = 0; i < config.numColors; ++i)
                {
                    if (!used[i])
                    {
                        used[i] = true;
                        current[pos] = static_cast<uint8_t>(i);
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

    // Calculate best guesses sorted by information value with multi-depth entropy
    std::vector<PatternGuess> calculateBestGuesses(
        const std::vector<Pattern> &allPatterns,
        const std::vector<Pattern> &possiblePatterns,
        const std::vector<Feedback> &guessHistory,
        const Config &config,
        int recursionLevel)
    {
        std::vector<PatternGuess> guesses;

        // Create a set for fast O(1) lookups.
        std::unordered_set<std::string> possiblePatternSet;
        for (const auto &p : possiblePatterns)
        {
            possiblePatternSet.insert(p.toString());
        }

        // Calculate entropy for each potential guess
        for (const auto &pattern : allPatterns)
        {
            PatternGuess guess;
            guess.pattern = pattern;

            // Initialize entropy levels
            std::vector<double> entropyList(config.maxDepth, 0.0);
            double firstLevelEntropy = 0.0;

            // Generate all possible feedback patterns for this guess
            std::map<Feedback, int> feedbackCounts;
            for (const auto &target : possiblePatterns)
            {
                Feedback fb = generateFeedback(target, pattern);
                feedbackCounts[fb]++;
            }

            // Calculate first level entropy and prepare for deeper levels
            for (const auto &pair : feedbackCounts)
            {
                double probability = static_cast<double>(pair.second) / possiblePatterns.size();
                double info = bits(probability);

                if (probability > 0)
                {
                    firstLevelEntropy += probability * info;

                    // Calculate deeper entropy if maxDepth > 1
                    if (config.maxDepth > 1)
                    {
                        // Create guess history with this feedback
                        auto tempHistory = guessHistory;
                        tempHistory.push_back(pair.first);

                        std::vector<Pattern> filteredPatterns = filterPatterns(allPatterns, tempHistory);

                        if (!filteredPatterns.empty())
                        {
                            // Recursively calculate best guess for deeper levels
                            Config nextConfig = config;
                            nextConfig.maxDepth = config.maxDepth - 1;

                            std::vector<PatternGuess> nextBestGuesses = calculateBestGuesses(
                                allPatterns, filteredPatterns, tempHistory, nextConfig, recursionLevel + 1);

                            if (!nextBestGuesses.empty())
                            {
                                const PatternGuess &bestNextGuess = nextBestGuesses[0];
                                // Add weighted entropy from next levels (like mastermind)
                                for (int i = 0; i < config.maxDepth - 1 && i < bestNextGuess.entropyList.size(); i++)
                                {
                                    double additionalEntropy = -bits(possiblePatterns.size()) +
                                                               (bits(filteredPatterns.size()) + bestNextGuess.entropyList[i]);
                                    entropyList[i + 1] += probability * additionalEntropy;
                                }
                            }
                        }
                    }
                }
            }

            entropyList[0] = firstLevelEntropy;
            guess.entropy = firstLevelEntropy;
            guess.entropyList = entropyList;

            // Calculate probability of this pattern being the answer
            bool isPossible = possiblePatternSet.count(pattern.toString());
            guess.probability = isPossible ? (1.0 / possiblePatterns.size()) : 0.0;

            guesses.push_back(guess);
        }

        // Sort by entropy levels (highest priority first)
        std::sort(guesses.begin(), guesses.end());

        // For recursive calls, only return the best guess to save computation
        if (recursionLevel > 0 && !guesses.empty())
        {
            return {guesses[0]};
        }

        return guesses;
    }

    // Enhanced solver that returns best guesses ranked by entropy and possible pattern count
    Result runMastermindSolverWithEntropy(
        const std::vector<Pattern> &allPatterns,
        const std::vector<Feedback> &guessHistory,
        const Config &config)
    {
        Result result;

        // First filter patterns based on existing feedback
        std::vector<Pattern> possiblePatterns = filterPatterns(allPatterns, guessHistory);
        result.totalPossiblePatterns = possiblePatterns.size();

        if (config.maxDepth == 0)
        {
            // Skip entropy calculation, just return the possible patterns as guesses
            for (const auto &pattern : possiblePatterns)
            {
                PatternGuess guess;
                guess.pattern = pattern;
                guess.entropy = 0.0;
                guess.probability = 1.0 / possiblePatterns.size();
                result.sortedGuesses.push_back(guess);
            }
        }
        else
        {
            // Calculate best guesses with entropy for all patterns
            std::vector<PatternGuess> allGuesses = calculateBestGuesses(allPatterns, possiblePatterns, guessHistory, config);
            result.sortedGuesses = allGuesses;
        }

        return result;
    }
}
