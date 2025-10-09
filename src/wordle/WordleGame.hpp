#pragma once
#include "../game/Game.hpp"
#include "../wordle/wordle.hpp"
#include "../utils/utils.hpp"
#include <vector>

namespace Game
{
    class WordleGame : public IGame
    {
    private:
        const std::vector<Utils::Word> &wordVec;

        // Helper methods
        Wordle::Config getConfigFromUser();
        Wordle::Config getConfigFromArgs(const std::map<std::string, std::string> &args);
        std::vector<Wordle::Feedback> getFeedbackFromUser();
        std::vector<Wordle::Feedback> getFeedbackFromArgs(const std::map<std::string, std::string> &args);
        void printResults(const Wordle::Result &result);
        void saveResults(const Wordle::Result &result, const std::string &possibleFile, const std::string &guessesFile);

    public:
        explicit WordleGame(const std::vector<Utils::Word> &words);

        void runCLI() override;
        void runHeadless(const std::map<std::string, std::string> &args) override;
        void runGUI() override;
        std::string getGameName() const override { return "wordle"; }
    };
}