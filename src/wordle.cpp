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
#include "utils.hpp"
#include "wordle.hpp"

namespace Wordle
{
    Feedback parseFeedback(const std::string &input)
    {
        Feedback fb;
        std::string word;
        std::string colors;
        size_t space = input.find(' ');
        if (space == std::string::npos || space + 6 > input.size())
            throw std::runtime_error("Invalid feedback format");
        word = input.substr(0, space);
        colors = input.substr(space + 1, 5);
        if (word.size() != 5 || colors.size() != 5)
            throw std::runtime_error("Word or colors wrong length");
        fb.word = WordUtils::trimToLower(word);
        for (int i = 0; i < 5; ++i)
        {
            if (colors[i] < '0' || colors[i] > '2')
                throw std::runtime_error("Invalid color digit");
            int color = colors[i] - '0';
            if (color == 0)
                fb.setGrey(i);
            else if (color == 1)
                fb.setYellow(i);
            else
                fb.setGreen(i);
        }
        return fb;
    }

    // Helper: Check if a word matches all feedback constraints
    bool matchesFeedback(const WordUtils::Word &candidate, const Feedback &fb)
    {
        const std::string &guess = fb.word;
        // Use pre-calculated letter count from candidate word
        std::array<uint8_t, 26> candidateLetterCount = candidate.letterCount;

        // Check greens first, and adjust counts
        for (int i = 0; i < 5; ++i)
        {
            if (fb.colors[i * 2 + 1]) // Green
            {                         // Green
                if (candidate.wordString[i] != guess[i])
                {
                    return false; // Must match green letters
                }
                candidateLetterCount[candidate.wordString[i] - 'a']--;
            }
        }

        for (int i = 0; i < 5; ++i)
        {
            char guessChar = guess[i];
            if (fb.colors[i * 2 + 1])
            { // Green (already checked)
                continue;
            }
            else if (fb.colors[i * 2])
            { // Yellow
                if (candidate.wordString[i] == guessChar)
                {
                    return false; // Yellow letter cannot be in the same spot
                }
                if (candidateLetterCount[guessChar - 'a'] <= 0)
                {
                    return false; // Must contain the yellow letter elsewhere
                }
                candidateLetterCount[guessChar - 'a']--;
            }
            else
            { // Grey
                if (candidateLetterCount[guessChar - 'a'] > 0)
                {
                    return false; // Candidate has a letter that was marked grey
                }
            }
        }

        return true;
    }

    // Generate feedback for a guess against a target word
    Feedback generateFeedback(const WordUtils::Word &target, const std::string &guess)
    {
        Feedback fb;
        fb.word = guess;

        // The bitset is already initialized to all zeros (all grey)
        std::array<uint8_t, 26> letterCount = target.letterCount;

        // First pass: Mark greens. A green is represented by setting both bits for a position to 1.
        // Example for position i: bit (i*2) = 1, bit (i*2 + 1) = 1.
        for (int i = 0; i < 5; ++i)
        {
            if (target.wordString[i] == guess[i])
            {
                // Direct manipulation: Set the 'green' bit and the 'yellow' bit.
                fb.colors[i * 2 + 1] = 1; // This bit signals "correct position" (Green).
                fb.colors[i * 2] = 1;     // This bit signals "in word" (Green or Yellow).

                letterCount[target.wordString[i] - 'a']--;
            }
        }

        // Second pass: Mark yellows. A yellow is when the "in word" bit is 1 but "correct position" is 0.
        for (int i = 0; i < 5; ++i)
        {
            if (!fb.colors[i * 2 + 1]) // If it's NOT green...
            {
                int guessCharIndex = guess[i] - 'a';
                if (letterCount[guessCharIndex] > 0)
                {
                    fb.colors[i * 2] = 1;
                    letterCount[guessCharIndex]--;
                }
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
    double calculateEntropy(const std::vector<WordUtils::Word> &possibleWords,
                            const WordUtils::Word &guess)
    {
        if (possibleWords.empty())
            return 0.0;

        std::unordered_map<unsigned long, int> feedbackCounts;

        // For each possible target word, generate feedback and count
        for (const auto &target : possibleWords)
        {
            Feedback fb = generateFeedback(target, guess.wordString);
            feedbackCounts[fb.colors.to_ulong()]++;
        }

        // Calculate entropy
        double entropy = 0.0;
        int totalWords = possibleWords.size();

        for (const auto &pair : feedbackCounts)
        {
            double probability = static_cast<double>(pair.second) / totalWords;
            entropy += probability * bits(probability);
        }

        return entropy;
    }

    std::vector<WordUtils::Word> filterWords(
        const std::vector<WordUtils::Word> &words,
        const std::vector<Feedback> &feedbacks)
    {
        std::vector<WordUtils::Word> filtered;
        for (const auto &w : words)
        {
            bool ok = true;
            for (const auto &fb : feedbacks)
            {
                if (!matchesFeedback(w, fb))
                {
                    ok = false;
                    break;
                }
            }
            if (ok)
                filtered.push_back(w);
        }
        return filtered;
    }

    // Calculate best guesses sorted by information value with multi-depth entropy
    std::vector<WordGuess> calculateBestGuesses(
        const std::vector<WordUtils::Word> &fiveLetterWords,
        const std::vector<WordUtils::Word> &possibleWords,
        const std::vector<Feedback> &feedbackHistory,
        const Config &config,
        int recursionLevel)
    {
        std::vector<WordGuess> guesses;

        // Create a set for fast O(1) lookups.
        std::unordered_set<std::string> possibleWordSet;
        for (const auto &w : possibleWords)
        {
            possibleWordSet.insert(w.wordString);
        }

        // Calculate entropy for each potential guess
        for (const auto &word : fiveLetterWords)
        {
            WordGuess guess;
            guess.word = word;

            // Initialize entropy levels
            std::vector<double> entropyList(config.maxDepth, 0.0);
            double firstLevelEntropy = 0.0;

            // Generate all possible feedback patterns for this guess
            std::unordered_map<unsigned long, int> feedbackCounts;
            for (const auto &target : possibleWords)
            {
                Feedback fb = generateFeedback(target, word.wordString);
                feedbackCounts[fb.colors.to_ulong()]++;
            }

            // Calculate first level entropy and prepare for deeper levels
            for (const auto &pair : feedbackCounts)
            {
                double probability = static_cast<double>(pair.second) / possibleWords.size();
                double info = bits(probability);

                // Can continue if probability is 0, but that shouldn't occur so we can skip the check

                firstLevelEntropy += probability * info;

                // Calculate deeper entropy if maxDepth > 1
                if (config.maxDepth > 1)
                {
                    // Create feedback pattern to filter words
                    Feedback dummyFeedback;
                    dummyFeedback.word = word.wordString;
                    dummyFeedback.colors = std::bitset<10>(pair.first);

                    std::vector<Feedback> tempFeedbacks = feedbackHistory;
                    tempFeedbacks.push_back(dummyFeedback);
                    std::vector<WordUtils::Word> filteredWords = filterWords(fiveLetterWords, tempFeedbacks);

                    if (!filteredWords.empty())
                    {
                        // Recursively calculate best guess for deeper levels
                        Config nextConfig = config;
                        nextConfig.maxDepth = config.maxDepth - 1;

                        std::vector<WordGuess> nextBestGuesses = calculateBestGuesses(
                            fiveLetterWords, filteredWords, tempFeedbacks, nextConfig, recursionLevel + 1);

                        if (!nextBestGuesses.empty())
                        {
                            const WordGuess &bestNextGuess = nextBestGuesses[0];
                            for (int i = 0; i < config.maxDepth - 1 && i < bestNextGuess.entropyList.size(); i++)
                            {
                                double additionalEntropy = -bits(possibleWords.size()) +
                                                           (bits(filteredWords.size()) + bestNextGuess.entropyList[i]);
                                entropyList[i + 1] += probability * additionalEntropy;
                            }
                        }
                    }
                }
            }

            entropyList[0] = firstLevelEntropy;
            guess.entropy = firstLevelEntropy;
            guess.entropyList = entropyList;

            // Calculate probability of this word being the answer
            bool isPossible = possibleWordSet.count(word.wordString);
            guess.probability = isPossible ? (1.0 / possibleWords.size()) : 0.0;

            guesses.push_back(guess);
        }

        // Sort by entropy levels (highest priority first) like mastermind
        std::sort(guesses.begin(), guesses.end(), [](const WordGuess &a, const WordGuess &b)
                  {
            // Compare entropy levels from last to first (highest depth first)
            for (int i = std::min(a.entropyList.size(), b.entropyList.size()) - 1; i >= 0; i--)
            {
                if (a.entropyList[i] != b.entropyList[i])
                    return a.entropyList[i] > b.entropyList[i];
            }
            // If all entropy levels are equal, prefer higher probability
            return a.probability > b.probability; });

        // For recursive calls, only return the best guess to save computation
        if (recursionLevel > 0 && !guesses.empty())
        {
            return {guesses[0]};
        }

        return guesses;
    }

    std::vector<WordUtils::Word> runWordleSolver(
        const std::vector<WordUtils::Word> &words,
        const std::vector<Feedback> &feedbacks)
    {
        return filterWords(words, feedbacks);
    }

    // Enhanced solver that returns best guesses ranked by entropy and possible word count
    Result runWordleSolverWithEntropy(
        const std::vector<WordUtils::Word> &allWords,
        const std::vector<Feedback> &feedbacks,
        const Config &config)
    {
        // Filter words to only 5-letter words
        std::vector<WordUtils::Word> availableWords;
        for (const auto &word : allWords)
        {
            bool exclude = config.excludeUncommonWords && (word.order >= 2);
            if (word.wordString.size() == 5 && !exclude)
                availableWords.push_back(word);
        }

        Result result;

        // First filter words based on existing feedback
        std::vector<WordUtils::Word> possibleWords = filterWords(availableWords, feedbacks);
        result.totalPossibleWords = possibleWords.size();

        std::vector<WordGuess> allGuesses;

        if (config.maxDepth == 0)
        {
            for (const auto &word : possibleWords)
            {
                WordGuess guess;
                guess.word = word;
                guess.entropy = 0.0;
                guess.probability = 1.0 / possibleWords.size();
                allGuesses.push_back(guess);
            }
        }
        else
        {
            // Calculate best guesses with entropy for all 5-letter words
            allGuesses = calculateBestGuesses(availableWords, possibleWords, feedbacks, config);
        }

        std::sort(allGuesses.begin(), allGuesses.end());

        result.sortedGuesses = allGuesses;
        return result;
    }
}