#pragma once

#include <vector>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <cmath>
#include <unordered_set>
#include <functional>

namespace Utils
{
    /**
     * Abstract base class for entropy-based puzzle solvers.
     * Implements the generic entropy calculation algorithm.
     * Template parameters:
     * - TCandidateType: The type representing a candidate solution (e.g., Word, Pattern)
     * - TFeedbackType: The type representing feedback from a guess
     * - TConfigType: Configuration type for the solver
     * - TGuessType: The specific guess type to return (e.g., WordGuess, PatternGuess)
     * - TResultType: The specific result type to return (e.g., Wordle::Result, Mastermind::Result)
     */
    template <typename TCandidateType, typename TFeedbackType, typename TConfigType, typename TGuessType, typename TResultType>
    class AbstractEntropySolver
    {
    public:
        virtual ~AbstractEntropySolver() = default;

    protected:
        // Pure virtual methods to be implemented by derived classes
        virtual bool matchesFeedback(const TCandidateType &candidate, const TFeedbackType &feedback) const = 0;
        virtual TFeedbackType generateFeedback(const TCandidateType &target, const TCandidateType &guess) const = 0;

        // Pure virtual methods for creating result objects directly
        virtual TGuessType createGuess(const TCandidateType &candidate, double entropy, double probability, const std::vector<double> &entropyList) const = 0;
        virtual TResultType createResult(const std::vector<TGuessType> &guesses, int totalPossible) const = 0;

    public:
        /**
         * Main solver function that returns the final result type directly
         */
        TResultType solve(
            const std::vector<TCandidateType> &allCandidates,
            const std::vector<TFeedbackType> &feedbackHistory,
            const TConfigType &config)
        {
            // Filter candidates based on feedback history
            std::vector<TCandidateType> possibleCandidates;

            // Create unordered_set using std::hash and std::equal_to (default template specializations)
            std::unordered_set<TCandidateType> possibleCandidateSet;

            for (const auto &candidate : allCandidates)
            {
                bool matches = true;
                for (const auto &feedback : feedbackHistory)
                {
                    if (!matchesFeedback(candidate, feedback))
                    {
                        matches = false;
                        break;
                    }
                }
                if (matches)
                {
                    possibleCandidates.push_back(candidate);
                    possibleCandidateSet.insert(candidate);
                }
            }

            std::vector<TGuessType> guesses;
            int totalPossible = static_cast<int>(possibleCandidates.size());

            // If maxDepth is 0, skip entropy calculation and just return filtered candidates
            if (config.maxDepth == 0)
            {
                for (const auto &candidate : possibleCandidates)
                {
                    double probability = possibleCandidates.empty() ? 0.0 : 1.0 / possibleCandidates.size();
                    TGuessType guess = createGuess(candidate, 0.0, probability, {});
                    guesses.push_back(guess);
                }
                return createResult(guesses, totalPossible);
            }

            // Calculate entropy for all candidates
            for (const auto &guessCandidate : allCandidates)
            {
                // Calculate probability (how likely this candidate is to be the answer)
                // O(1) lookup in unordered_set with proper hash and equality
                bool isPossible = possibleCandidateSet.find(guessCandidate) != possibleCandidateSet.end();

                double probability = isPossible ? (possibleCandidates.empty() ? 0.0 : 1.0 / possibleCandidates.size()) : 0.0;

                // Calculate entropy at different depths
                std::vector<double> entropyList = calculateEntropyAtDepths(guessCandidate, possibleCandidates, config.maxDepth);
                double entropy = entropyList.empty() ? 0.0 : entropyList.back();

                TGuessType guess = createGuess(guessCandidate, entropy, probability, entropyList);
                guesses.push_back(guess);
            }

            // Sort by entropy (highest first) - assumes TGuessType has operator<
            std::sort(guesses.begin(), guesses.end());

            return createResult(guesses, totalPossible);
        }

    private:
        /**
         * Calculate entropy at multiple depths efficiently in a single calculation
         * Returns vector where entropyList[i] = entropy at depth (i+1)
         * This is much more efficient than calling calculateEntropyAtDepth multiple times
         */
        std::vector<double> calculateEntropyAtDepths(const TCandidateType &guessCandidate,
                                                     const std::vector<TCandidateType> &possibleCandidates,
                                                     int maxDepth)
        {
            std::vector<double> entropyList(maxDepth, 0.0);

            if (possibleCandidates.empty() || maxDepth <= 0)
                return entropyList;

            // Group possible candidates by feedback pattern
            std::unordered_map<TFeedbackType, std::vector<TCandidateType>> feedbackGroups;

            for (const auto &target : possibleCandidates)
            {
                TFeedbackType feedback = generateFeedback(target, guessCandidate);
                feedbackGroups[feedback].push_back(target);
            }

            // Calculate base entropy (depth 1)
            double total = static_cast<double>(possibleCandidates.size());
            for (const auto &group : feedbackGroups)
            {
                double prob = static_cast<double>(group.second.size()) / total;
                if (prob > 0.0)
                {
                    entropyList[0] -= prob * std::log2(prob);
                }
            }

            // Calculate deeper entropy levels if needed
            for (int depth = 2; depth <= maxDepth; ++depth)
            {
                double totalDeeperEntropy = 0.0;

                for (const auto &group : feedbackGroups)
                {
                    if (group.second.size() > 1) // Only calculate if there are multiple candidates
                    {
                        double prob = static_cast<double>(group.second.size()) / total;

                        // Find best next guess for this subgroup at remaining depth
                        double bestSubEntropy = 0.0;
                        for (const auto &nextGuess : possibleCandidates)
                        {
                            // Recursively calculate entropy for remaining depth
                            std::vector<double> subEntropyList = calculateEntropyAtDepths(
                                nextGuess, group.second, depth - 1);

                            // Take the last (deepest) entropy level
                            if (!subEntropyList.empty())
                            {
                                bestSubEntropy = std::max(bestSubEntropy, subEntropyList.back());
                            }
                        }

                        totalDeeperEntropy += prob * bestSubEntropy;
                    }
                }

                entropyList[depth - 1] = totalDeeperEntropy;
            }

            return entropyList;
        }
    };
}