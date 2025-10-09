#include <chrono>
#include <string>
#include <regex>
#include <iostream>
#include <cmath>
#include <vector>
#include <fstream>
#include <map>
#include <iomanip>
#include <filesystem>
#include <set>
#include <sstream>

#include "utils.hpp"

namespace ProfilerUtils
{
    constexpr double NANO_TO_SEC = 1.0 / 1000000000;
    constexpr int SEC_TO_NANO = 1000000000;

    // returns time in nanoseconds
    int64_t getTime()
    {
        return (std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::system_clock::now()).time_since_epoch()).count());
    }

    std::string getDatetime(int plusSeconds)
    {
        std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
        time_t now_c = std::chrono::system_clock::to_time_t(now + std::chrono::seconds(plusSeconds));

        time_t tt;
        struct tm *ti;
        time(&tt);
        // ti = localtime(&tt);
        ti = localtime(&now_c);
        std::string date = asctime(ti);
        date = std::regex_replace(date, std::regex("\n"), "");
        return (date);
    }

    // --- Process ---
    Process::Process()
    {
        lastPrint = getTime();
        this->lastMessageSize = 0;
    }

    void Process::printUpdate(std::string message)
    {
        int64_t time = getTime();
        lastPrint = time;
        clearLine();
        std::cout << message;
        lastMessageSize = message.size();
    }

    void Process::clearLine()
    {
        std::cout << "\r" << std::string(lastMessageSize + 1, ' ') << "\r";
    }

    void Process::start()
    {
        this->startTime = getTime();
        lastPrint = this->startTime;
    }

    std::string Process::formatSeconds(int64_t totalNano)
    {

        int days = totalNano / 86400000000000;
        totalNano = totalNano % 86400000000000;

        int hours = totalNano / 3600000000000;
        totalNano = totalNano % 3600000000000;

        int minutes = totalNano / 60000000000;
        totalNano = totalNano % 60000000000;

        double seconds = totalNano / 1000000000.0;

        std::string s = "";
        if (days > 0)
        {
            s.append(std::format("{}d ", days));
        }
        if (hours > 0)
        {
            s.append(std::format("{}h ", hours));
        }
        if (minutes > 0)
        {
            s.append(std::format("{}m ", minutes));
        }
        s.append(std::format("{:.0f}s ", seconds));

        return (s);
    }

    int64_t Process::getTimeRemaining(double progress)
    {
        return (((getTime() - startTime) / (progress - 0)) * (1 - progress));
    }

    void Process::update(double progress, int delay)
    {
        if (progress <= 0)
        {
            progress = 0;
        }
        int64_t time = getTime();
        if (time - lastPrint > (delay * SEC_TO_NANO))
        {
            printUpdate(std::format("Progress: {:.2f}% Time remaining: {}",
                                    progress * 100,
                                    formatSeconds(getTimeRemaining(progress))));
            lastPrint = time;
        }
    }

    // --- functionProfile ---
    FunctionProfile::FunctionProfile() {}

    FunctionProfile::FunctionProfile(const std::string &name, FunctionProfile *parent)
    {
        this->functionName = name;
        this->parent = parent;
        this->childProfileMap = std::unordered_map<std::string, FunctionProfile>();
        this->childList = std::vector<FunctionProfile *>();
        this->startTime = 0;
        this->count = 0;
        this->totalTime = 0;
        this->recursionDepth = 0;
        this->maxRecursionDepth = 0;
    }

    // --- Profiler ---
    Profiler::Profiler()
    {
        this->start();
    }

    void Profiler::log(const std::string &message)
    {
        std::ofstream logFile;
        if (this->logDirectory == "")
        {
            logFile.open("log.txt", std::ios::app);
        }
        else
        {
            logFile.open(this->logDirectory + "\\log.txt", std::ios::app);
        }
        logFile << std::fixed << std::setprecision(9);
        logFile << message << "\n";
        logFile.close();
    }

    void Profiler::profileStart(const std::string &functionName)
    {
        int64_t currentTime = getTime();
        // Check if this is a recursive call (same function as current)
        // This optimization prevents creating duplicate child profiles for recursive functions
        if (currentProfile->functionName == functionName)
        {
            // Recursive call - don't create new profile, just increment count
            // This saves memory and improves performance for recursive algorithms
            currentProfile->count++;
            currentProfile->recursionDepth++;
            if (currentProfile->recursionDepth > currentProfile->maxRecursionDepth)
            {
                currentProfile->maxRecursionDepth = currentProfile->recursionDepth;
            }
        }
        else
        {
            // Normal function start - find or create child profile
            FunctionProfile *targetProfile = nullptr;

            // Try to find existing child profile
            auto it = currentProfile->childProfileMap.find(functionName);
            if (it != currentProfile->childProfileMap.end())
            {
                targetProfile = &it->second;
            }
            else
            {
                // Create new child profile
                auto insertIt = currentProfile->childProfileMap.emplace_hint(
                    it, functionName, FunctionProfile(functionName, currentProfile));
                targetProfile = &insertIt->second;

                currentProfile->childList.push_back(&insertIt->second);
            }

            // Start timing for this function
            targetProfile->startTime = currentTime;

            // Move down the tree to the new current function
            currentProfile = targetProfile;
        }
        this->profilerUpdater->count++;
        this->profilerUpdater->totalTime += getTime() - currentTime;
    }

    void Profiler::profileEnd(const std::string &functionName)
    {
        int64_t currentTime = getTime();
        // Function end - check if we're ending the current function
        if (currentProfile->functionName == functionName)
        {
            if (currentProfile->recursionDepth > 0)
            {
                // Decrement recursion depth if applicable
                currentProfile->recursionDepth--;
            }
            else
            {
                // Only update timing if this is the root call
                currentProfile->count++;
                // Use getTime() here to avoid small timing errors
                currentProfile->totalTime += getTime() - currentProfile->startTime;
                currentProfile->startTime = 0;

                // Move back up the tree to the parent function
                currentProfile = currentProfile->parent;
            }
        }
        this->profilerUpdater->count++;
        this->profilerUpdater->totalTime += getTime() - currentTime;
    }

    void Profiler::start()
    {
        this->startTime = getTime();
        this->main = FunctionProfile("[MAIN]", nullptr);

        auto [insertIt, success] = this->main.childProfileMap.emplace(
            "[PROFILER]", FunctionProfile("[PROFILER]", &main));
        this->profilerUpdater = &insertIt->second;
        this->main.childList.push_back(this->profilerUpdater);

        this->currentProfile = &this->main; // Start at the main profile
        this->running = true;
    }

    void Profiler::end()
    {
        this->endTime = getTime();

        // Clean up any remaining functions by walking back up the tree
        while (currentProfile != &main && currentProfile != nullptr)
        {
            if (currentProfile->startTime > 0)
            {
                currentProfile->totalTime += this->endTime - currentProfile->startTime;
                currentProfile->count++;
                currentProfile->startTime = 0;
            }
            currentProfile = currentProfile->parent;
        }
        this->running = false;
    }

    void Profiler::logProfile(FunctionProfile &profile, int64_t totalTime, int depth, bool corner)
    {
        FunctionProfile *parent_ptr = profile.parent;
        if (parent_ptr == nullptr)
        {
            parent_ptr = &profile; // Avoid null dereference for root
        }
        FunctionProfile &parent = *parent_ptr;

        uint32_t indentNumChars = depth * 4;
        std::string indent = "";
        if (depth > 0)
        {
            indent = std::string((depth - 1) * 4, ' ');
            if (corner)
                indent += "  └─";
            else
                indent += "  ├─";
        }

        double average = (profile.count == 0) ? 0.0 : (static_cast<double>(profile.totalTime) / profile.count);
        double averageMs = average * NANO_TO_SEC * 1000.0;
        double totalSec = profile.totalTime * NANO_TO_SEC;
        double relativePercent = (parent.totalTime == 0) ? 0.0 : (100.0 * static_cast<double>(profile.totalTime) / parent.totalTime);
        double totalPercent = (totalTime == 0) ? 0.0 : (100.0 * static_cast<double>(profile.totalTime) / totalTime);

        uint32_t nameFieldWidth = 40;
        uint32_t nameStrWidth = nameFieldWidth - indentNumChars;

        std::string nameStr = profile.functionName;
        if (nameStr.size() > nameStrWidth)
            nameStr = nameStr.substr(0, nameStrWidth - 3) + "...";

        std::string nameField = indent + nameStr + std::string(nameStrWidth - nameStr.size(), ' ');

        // subsequent columns: each width 10
        log(std::format("{} {:>15.6f} {:>10} {:>10} {:>10.4f} {:>9.2f}% {:>9.2f}%",
                        nameField,
                        averageMs,
                        profile.count,
                        profile.maxRecursionDepth + 1, // +1 to count the initial call
                        totalSec,
                        relativePercent,
                        totalPercent));

        uint32_t numChild = profile.childList.size();
        for (uint32_t i = 0; i < numChild; i++)
        {
            FunctionProfile &child = *profile.childList[i];
            corner = (i == numChild - 1) || !child.childProfileMap.empty();
            logProfile(child, totalTime, depth + 1, corner);
        }
    }

    void Profiler::logProfilerData()
    {
        int64_t totalRunTime = (this->endTime) - (this->startTime);
        this->main.totalTime = totalRunTime;
        this->main.count = 1;

        log(std::format("{:<40} {:>15} {:>10} {:>10} {:>10} {:>10} {:>10}",
                        "FUNCTION",
                        "AVG (ms)",
                        "COUNT",
                        "DEPTH",
                        "TOTAL (s)",
                        "% PARENT",
                        "% TOTAL"));
        logProfile(main, totalRunTime);
        log("");
    }

    int64_t Profiler::getTotalTime()
    {
        return this->endTime - this->startTime;
    }

    ProfileScope::ProfileScope(Profiler &profiler, const std::string &name)
        : profiler(profiler), functionName(name)
    {
        if (!profiler.isRunning())
            return;
        profiler.profileStart(functionName);
    }

    ProfileScope::~ProfileScope()
    {
        if (!profiler.isRunning())
            return;
        profiler.profileEnd(functionName);
    }

    ProfilerUtils::Profiler g_profiler;
} // namespace Utils

namespace WordUtils
{
    std::string trimToLower(const std::string &str)
    {
        std::string trimmed = str;
        trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), [](unsigned char ch)
                                                    { return !std::isspace(ch); }));
        trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char ch)
                                   { return !std::isspace(ch); })
                          .base(),
                      trimmed.end());
        std::transform(trimmed.begin(), trimmed.end(), trimmed.begin(), ::tolower);
        return trimmed;
    }

    // Loads words from words.bin if available, otherwise from word_scores.csv (first 300,000 words) and saves to words.bin.
    std::vector<Word> loadWords()
    {
        std::filesystem::path data_dir = std::filesystem::current_path() / "data";
        std::filesystem::path csv_file = data_dir / "word_scores.csv";
        std::filesystem::path bin_file = data_dir / "words.bin";
        std::vector<Word> allWordsVec;

        // Try to load from binary file first
        std::ifstream in(bin_file, std::ios::binary);
        bool loadedFromBin = false;
        if (in)
        {
            try
            {
                size_t n;
                in.read(reinterpret_cast<char *>(&n), sizeof(n));
                allWordsVec.resize(n);
                for (size_t i = 0; i < n; ++i)
                {
                    size_t len;
                    in.read(reinterpret_cast<char *>(&len), sizeof(len));
                    allWordsVec[i].wordString.resize(len);
                    in.read(&allWordsVec[i].wordString[0], len);
                    in.read(reinterpret_cast<char *>(&allWordsVec[i].score), sizeof(allWordsVec[i].score));
                    in.read(reinterpret_cast<char *>(&allWordsVec[i].is_scrabble), sizeof(allWordsVec[i].is_scrabble));
                    in.read(reinterpret_cast<char *>(&allWordsVec[i].uniqueLetters), sizeof(allWordsVec[i].uniqueLetters));
                    in.read(reinterpret_cast<char *>(&allWordsVec[i].letterCount), sizeof(allWordsVec[i].letterCount));
                    if (!in)
                        throw std::runtime_error("Read error");
                }
                loadedFromBin = true;
                in.close();
            }
            catch (...)
            {
                in.close();
                allWordsVec.clear();
            }
        }

        // If binary loading failed, load from CSV
        if (!loadedFromBin)
        {
            std::ifstream file(csv_file);
            if (!file.is_open())
            {
                std::cerr << "Error: Could not open word_scores.csv. Please ensure it's in the 'data' directory.\n";
                return allWordsVec;
            }

            std::string line;
            // Skip header line
            if (!std::getline(file, line))
            {
                std::cerr << "Error: Empty CSV file.\n";
                file.close();
                return allWordsVec;
            }

            constexpr size_t MAX_WORDS = 500002;
            size_t wordCount = 0;

            while (std::getline(file, line) && wordCount < MAX_WORDS)
            {
                if (line.empty())
                    continue;

                // Parse CSV line: word,is_scrabble,final_score
                std::istringstream ss(line);
                std::string word, is_scrabble_str, score_str;

                if (std::getline(ss, word, ',') &&
                    std::getline(ss, is_scrabble_str, ',') &&
                    std::getline(ss, score_str))
                {
                    // Clean and validate word
                    word = trimToLower(word);
                    if (word.empty() || std::any_of(word.begin(), word.end(), [](unsigned char c)
                                                    { return !std::isalpha(c); }))
                    {
                        continue;
                    }

                    // Parse is_scrabble and score
                    bool is_scrabble = (is_scrabble_str == "1");
                    double score;
                    try
                    {
                        score = std::stod(score_str);
                    }
                    catch (...)
                    {
                        continue; // Skip invalid score entries
                    }

                    // Calculate unique letters count
                    int uniqueLetters = std::set<char>(word.begin(), word.end()).size();

                    // Calculate letter count array
                    std::array<uint8_t, 26> letterCount = {0};
                    for (char c : word)
                    {
                        letterCount[c - 'a']++;
                    }

                    allWordsVec.push_back({word, score, is_scrabble, uniqueLetters, letterCount});
                    wordCount++;
                }
            }
            file.close();

            // Save to binary for next time
            if (!std::filesystem::exists(data_dir))
            {
                std::filesystem::create_directories(data_dir);
            }

            std::ofstream out(bin_file, std::ios::binary);
            if (out)
            {
                size_t n = allWordsVec.size();
                out.write(reinterpret_cast<const char *>(&n), sizeof(n));
                for (const auto &w : allWordsVec)
                {
                    size_t len = w.wordString.size();
                    out.write(reinterpret_cast<const char *>(&len), sizeof(len));
                    out.write(w.wordString.data(), len);
                    out.write(reinterpret_cast<const char *>(&w.score), sizeof(w.score));
                    out.write(reinterpret_cast<const char *>(&w.is_scrabble), sizeof(w.is_scrabble));
                    out.write(reinterpret_cast<const char *>(&w.uniqueLetters), sizeof(w.uniqueLetters));
                    out.write(reinterpret_cast<const char *>(&w.letterCount), sizeof(w.letterCount));
                }
                out.close();
            }
        }

        return allWordsVec;
    }
} // namespace WordUtils