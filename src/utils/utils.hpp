#pragma once
#include <string>
#include <map>
#include <vector>
#include <array>
#include <cstdint>
#include <unordered_map>

namespace ProfilerUtils
{

    long long getTime();

    std::string getDatetime(int plusSeconds = 0);

    class Process
    {
    public:
        Process();
        void printUpdate(std::string message);
        void clearLine();
        void start();
        std::string formatSeconds(int64_t totalNano);
        int64_t getTimeRemaining(double progress);
        void update(double progress, int delay = 1);

    private:
        int64_t startTime;
        int64_t lastPrint;
        int lastMessageSize;
    };

    class FunctionProfile
    {
    public:
        std::string functionName;
        std::unordered_map<std::string, FunctionProfile> childProfileMap;
        std::vector<FunctionProfile *> childList;
        FunctionProfile *parent;
        int64_t startTime;
        unsigned int count;
        int64_t totalTime;
        unsigned int recursionDepth;
        unsigned int maxRecursionDepth;

        FunctionProfile();
        FunctionProfile(const std::string &name, FunctionProfile *parent);
    };

    class Profiler
    {
    public:
        FunctionProfile *profilerUpdater;
        FunctionProfile main;
        FunctionProfile *currentProfile; // Current function being profiled (top of call tree)
        std::string logDirectory;
        int64_t startTime;
        int64_t endTime;
        bool running = false;

        Profiler();
        void log(const std::string &message);
        void profileStart(const std::string &functionName);
        void profileEnd(const std::string &functionName);
        void start();
        void end();
        void logProfilerData();
        void logProfile(FunctionProfile &profile, int64_t totalTime, int depth = 0, bool corner = false);
        int64_t getTotalTime();
        bool isRunning() { return running; }
    };

    class ProfileScope
    {
    public:
        ProfileScope(Profiler &profiler, const std::string &name);

        ~ProfileScope();

        // Prevent copying and moving to ensure RAII integrity
        ProfileScope(const ProfileScope &) = delete;
        ProfileScope &operator=(const ProfileScope &) = delete;

    private:
        Profiler &profiler;
        std::string functionName;
        bool ignored;
    };

    extern ProfilerUtils::Profiler g_profiler;

} // namespace Utils

namespace WordUtils
{
    struct Word
    {
        std::string wordString;
        double score;
        bool is_scrabble;
        int uniqueLetters;
        std::array<uint8_t, 26> letterCount; // Count of each letter a-z
        bool operator<(const Word &other) const { return wordString < other.wordString; }
    };

    std::string trimToLower(const std::string &str);

    std::vector<Word> loadWords();

    // Other utility functions related to words can be declared here
} // namespace WordUtils