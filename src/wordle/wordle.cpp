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

#include "wordle.hpp"
#include "../utils/EntropySolver.hpp"

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
        fb.word = Utils::trimToLower(word);
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
    bool matchesFeedback(const Utils::Word &candidate, const Feedback &fb)
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
    Feedback generateFeedback(const Utils::Word &target, const std::string &guess)
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
    double calculateEntropy(const std::vector<Utils::Word> &possibleWords,
                            const Utils::Word &guess)
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

    std::vector<Utils::Word> filterWords(
        const std::vector<Utils::Word> &words,
        const std::vector<Feedback> &feedbacks)
    {
        std::vector<Utils::Word> filtered;
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

    std::vector<Utils::Word> runWordleSolver(
        const std::vector<Utils::Word> &words,
        const std::vector<Feedback> &feedbacks)
    {
        return filterWords(words, feedbacks);
    }

    // Wordle-specific entropy solver implementation
    class WordleEntropySolver : public Utils::AbstractEntropySolver<Utils::Word, Feedback, Config, WordGuess, Result>
    {
    protected:
        bool matchesFeedback(const Utils::Word &candidate, const Feedback &feedback) const override
        {
            return Wordle::matchesFeedback(candidate, feedback);
        }

        Feedback generateFeedback(const Utils::Word &target, const Utils::Word &guess) const override
        {
            return Wordle::generateFeedback(target, guess.wordString);
        }

        WordGuess createGuess(const Utils::Word &candidate, double entropy, double probability, const std::vector<double> &entropyList) const override
        {
            WordGuess guess;
            guess.word = candidate;
            guess.entropy = entropy;
            guess.probability = probability;
            guess.entropyList = entropyList;
            return guess;
        }

        Result createResult(const std::vector<WordGuess> &guesses, int totalPossible) const override
        {
            Result result;
            result.sortedGuesses = guesses;
            result.totalPossibleWords = totalPossible;
            return result;
        }
    };

    Result runWordleSolver(
        const std::vector<Utils::Word> &allWords,
        const std::vector<Feedback> &feedbacks,
        const Config &config)
    {
        // Filter words to only 5-letter words
        std::vector<Utils::Word> availableWords;
        for (const auto &word : allWords)
        {
            bool exclude = config.excludeUncommonWords && (!word.is_scrabble);
            if (word.wordString.length() == 5 && !exclude)
            {
                availableWords.push_back(word);
            }
        }

        // Use the specialized Wordle entropy solver - returns Result directly!
        WordleEntropySolver solver;
        return solver.solve(availableWords, feedbacks, config);
    }
}