#pragma once
#include <string>
#include <map>
#include <vector>
#include <array>
#include <cstdint>
#include <unordered_map>

namespace Utils
{

    long long getTime();

    std::string getDatetime(const int plusSeconds = 0);

    /*
     * A simple utility class to manage and display progress updates in the console.
     */
    class Process
    {
    public:
        Process();
        // Print a message to the console, overwriting the previous message
        void printUpdate(const std::string &message);
        // Clear the current line in the console
        void clearLine();
        // Set the start time for the process
        void start();
        // Clear the current line in the console
        void stop();
        // Format a duration in nanoseconds into a human-readable string
        std::string formatSeconds(int64_t totalNano) const;
        // Estimate the remaining time (in nanoseconds) based on progress (0-1)
        int64_t getTimeRemaining(const double progress) const;
        // Update the progress display if enough time has passed (delay in seconds)
        void update(double progress, double delay = 1.0);

    private:
        int64_t startTime;
        int64_t lastPrint;
        int lastMessageSize;
    };

    class FunctionProfile
    {
    public:
        FunctionProfile(const std::string &name, FunctionProfile *parent);

        // Public API
        void reset();

        // Get name of the function being profiled
        const std::string &getName() const { return functionName; }
        // Get total time spent in this function (including all calls)
        int64_t getTotalTime() const { return totalTime; }
        // Get number of times this function was called
        unsigned int getCount() const { return count; }
        // Get maximum recursion depth reached for this function
        unsigned int getMaxRecursionDepth() const { return maxRecursionDepth; }
        // Get current recursion depth for this function
        unsigned int getRecursionDepth() const { return recursionDepth; }
        // Get the parent function profile (nullptr for root)
        FunctionProfile *getParent() const { return parent; }
        // Get the list of child function profiles
        const std::vector<FunctionProfile *> &getChildren() const { return childList; }
        // Get the start time of the current invocation in nanoseconds
        int64_t getStartTime() const { return startTime; }

        // Add one to the count of times this function was called
        void incrementCount() { count++; }
        // Add time (in nanoseconds) to the total time spent in this function
        void addToTotalTime(int64_t time) { totalTime += time; }

        // Internal methods (should only be called by Profiler)
        void profileStart(int64_t now);
        void profileStop(int64_t now);
        FunctionProfile *getOrCreateChild(const std::string &name);
        void setTotalTime(int64_t time) { totalTime = time; }

    private:
        // The name of the function being profiled
        std::string functionName;
        // Map of child function profiles for quick lookup
        std::unordered_map<std::string, FunctionProfile> childProfileMap;
        // Ordered list of child profiles for consistent logging order
        std::vector<FunctionProfile *> childList;
        // Pointer to the parent function profile (nullptr for root)
        FunctionProfile *parent;
        // Start time of the current invocation in nanoseconds
        int64_t startTime;
        // Total number of times this function was called
        unsigned int count;
        // Total time spent in this function (including all calls)
        int64_t totalTime;
        // Current recursion depth for this function
        unsigned int recursionDepth;
        // Maximum recursion depth reached for this function
        unsigned int maxRecursionDepth;
    };

    /*
     * Profiler class to manage and log profiling data for function calls.
     */
    class Profiler
    {
    public:
        Profiler();

        // Public API
        void start();
        void stop();
        void log(const std::string &message);
        void profileStart(const std::string &functionName);
        void profileStop(const std::string &functionName);
        void logProfilerData();
        int64_t getTotalTime() const;
        bool isRunning() const { return running; }

        // Read-only access to main profile
        const FunctionProfile &getMainProfile() const { return main; }

    private:
        // Log a specific function profile and its children recursively
        void logProfile(const FunctionProfile &profile, const int64_t totalTime, const int depth = 0, const bool corner = false);

        // Pointer to the special "[PROFILER]" function profile for overhead tracking
        FunctionProfile *profilerUpdater;
        // The root function profile representing the whole process
        FunctionProfile main;
        // The current function being profiled (top of call tree)
        FunctionProfile *currentProfile;
        // The directory where log files are saved
        std::string logDirectory;
        // The start time of the profiling session
        int64_t startTime;
        // The end time of the profiling session
        int64_t endTime;
        // Whether the profiler is currently running
        bool running = false;
    };

    /*
     * RAII class to automatically profile a scope (function/block) using the Profiler.
     */
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
    };

    constexpr double NANO_TO_SEC = 1.0 / 1000000000;
    constexpr int SEC_TO_NANO = 1000000000;

    // Global profiler instance
    extern Profiler g_profiler;
    // Global process instance
    extern Process g_process;

} // namespace Utils