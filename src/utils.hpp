#pragma once
#include <string>
#include <map>
#include <vector>

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

    class functionProfile
    {
    public:
        std::string functionName;
        std::map<std::string, functionProfile> childProfileMap;
        std::vector<std::string> functionList;
        functionProfile *parent;
        double startTime;
        int count;
        double totalTime;

        functionProfile();
        functionProfile(const std::string &name, functionProfile *parent);
        void update(double time, functionProfile *&p);
    };

    class Profiler
    {
    public:
        functionProfile *profilerUpdater;
        functionProfile main;
        functionProfile *currentProfile;
        std::string logDirectory;
        double startTime;
        double endTime;

        Profiler();
        void log(const std::string &message);
        void updateProfile(const std::string &functionName, bool start);
        void profileStart(const std::string &functionName, bool ignore = false);
        void profileEnd(const std::string &functionName, bool ignore = false);
        void start();
        void end();
        void logChildProfiles(functionProfile &profile, int depth);
        void logProfilerData();
        double getTotalTime();
    };

} // namespace Utils

namespace WordUtils
{
    struct Word
    {
        std::string wordString;
        int order;
        int count;
        int uniqueLetters;
        bool operator<(const Word &other) const { return wordString < other.wordString; }
    };

    std::string trimToLower(const std::string &str);

    std::vector<Word> loadWords();

    // Other utility functions related to words can be declared here
} // namespace WordUtils