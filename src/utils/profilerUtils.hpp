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

    std::string getDatetime(int plusSeconds = 0);

    /*
     * A simple utility class to manage and display progress updates in the console.
     */
    class Process
    {
    public:
        Process();
        // Print a message to the console, overwriting the previous message
        void printUpdate(std::string message);
        // Clear the current line in the console
        void clearLine();
        // Set the start time for the process
        void start();
        // Clear the current line in the console
        void stop();
        // Format a duration in nanoseconds into a human-readable string
        std::string formatSeconds(int64_t totalNano);
        // Estimate the remaining time (in nanoseconds) based on progress (0-1)
        int64_t getTimeRemaining(double progress);
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

        FunctionProfile(const std::string &name, FunctionProfile *parent);

        // Reset the profile data (for reuse)
        void reset();
    };

    /*
     * Profiler class to manage and log profiling data for function calls.
     */
    class Profiler
    {
    public:
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

        Profiler();
        // Log a message to the profiler log
        void log(const std::string &message);
        // Record the start of a function call
        void profileStart(const std::string &functionName);
        // Record the end of a function call
        void profileStop(const std::string &functionName);
        // Start the profiler
        void start();
        // Stop the profiler
        void stop();
        // Log a summary of profiling data
        void logProfilerData();
        // Log a specific function profile and its children recursively
        void logProfile(FunctionProfile &profile, int64_t totalTime, int depth = 0, bool corner = false);
        // Get the total time of the profiling session in nanoseconds
        int64_t getTotalTime();
        // Check if the profiler is currently running
        bool isRunning() { return running; }
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