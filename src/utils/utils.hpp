#pragma once
#include <string>
#include <map>
#include <vector>
#include <array>
#include <cstdint>

namespace ProfilerUtils
{

    double getTime();

    std::string getDatetime(int plusSeconds = 0);

    class Process
    {
    public:
        Process();
        void printUpdate(std::string message);
        void clearLine();
        void start();
        std::string formatSeconds(double totalSeconds);
        double getTimeRemaining(double progress);
        void update(double progress, double delay = 1);

    private:
        double startTime;
        double lastPrint;
        int lastMessageSize;
    };

    class FunctionProfile
    {
    public:
        std::string functionName;
        std::map<std::string, FunctionProfile> childProfileMap;
        std::vector<std::string> functionList;
        FunctionProfile *parent;
        double startTime;
        unsigned int count;
        double totalTime;
        unsigned int recursionDepth;

        FunctionProfile();
        FunctionProfile(const std::string &name, FunctionProfile *parent);
        void update(double time, FunctionProfile *&p);
    };

    class Profiler
    {
    public:
        FunctionProfile *profilerUpdater;
        FunctionProfile main;
        FunctionProfile *currentProfile; // Current function being profiled (top of call tree)
        std::string logDirectory;
        double startTime;
        double endTime;
        bool running = false;

        Profiler();
        void log(const std::string &message);
        void updateProfile(const std::string &functionName, bool start);
        void profileStart(const std::string &functionName);
        void profileEnd(const std::string &functionName);
        void start();
        void end();
        void logChildProfiles(FunctionProfile &profile, int depth);
        void logProfilerData();
        double getTotalTime();
        double isRunning() { return running; }
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