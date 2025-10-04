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

    // returns time in seconds
    double getTime()
    {
        return (std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::system_clock::now()).time_since_epoch()).count() * NANO_TO_SEC);
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
        double time = getTime();
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

    std::string Process::formatSeconds(double totalSeconds)
    {
        int numSeconds = (int)totalSeconds;
        double decimal = totalSeconds - numSeconds;
        int days = numSeconds / 86400;
        numSeconds = numSeconds % 86400;

        int hours = numSeconds / 3600;
        numSeconds = numSeconds % 3600;

        int minutes = numSeconds / 60;
        numSeconds = numSeconds % 60;

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
        s.append(std::format("{:.0f}s ", numSeconds + decimal));

        return (s);
    }

    double Process::getTimeRemaining(double progress)
    {
        return (((getTime() - startTime) / (progress - 0)) * (1 - progress));
    }

    void Process::update(double progress, double delay)
    {
        if (progress <= 0)
        {
            progress = 0;
        }
        double time = getTime();
        if (time - lastPrint > delay)
        {
            printUpdate(std::format("Progress: {:.2f}% Time remaining: {}",
                                    progress * 100,
                                    formatSeconds(getTimeRemaining(progress))));
            lastPrint = time;
        }
    }

    // --- functionProfile ---
    functionProfile::functionProfile() {}

    functionProfile::functionProfile(const std::string &name, functionProfile *parent)
    {
        this->functionName = name;
        this->parent = parent;
        this->childProfileMap = std::map<std::string, functionProfile>();
        this->functionList = std::vector<std::string>();
        this->startTime = 0;
        this->count = 0;
        this->totalTime = 0;
    }

    void functionProfile::update(double time, functionProfile *&p)
    {
        if (this->startTime == 0)
        {
            this->startTime = getTime();
            p = this;
        }
        else
        {
            this->totalTime += time - this->startTime;
            this->count = this->count + 1;
            this->startTime = 0;
            p = this->parent;
        }
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

    void Profiler::updateProfile(const std::string &functionName, bool start)
    {
        double t = getTime();

        if (currentProfile->functionName == functionName && !start)
        {
            currentProfile->update(t, currentProfile);
        }
        else
        {
            try
            {
                functionProfile *p = &this->currentProfile->childProfileMap.at(functionName);
                p->update(t, currentProfile);
            }
            catch (...)
            {
                this->currentProfile->functionList.push_back(functionName);
                this->currentProfile->childProfileMap[functionName] = *new functionProfile(functionName, currentProfile);
                this->currentProfile->childProfileMap.at(functionName).update(t, currentProfile);
            }
        }
        this->profilerUpdater->count++;
        this->profilerUpdater->totalTime += getTime() - t;
    }

    void Profiler::profileStart(const std::string &functionName, bool ignore)
    {
        if (!ignore)
        {
            updateProfile(functionName, true);
        }
    }

    void Profiler::profileEnd(const std::string &functionName, bool ignore)
    {
        if (!ignore)
        {
            updateProfile(functionName, false);
        }
    }

    void Profiler::start()
    {
        this->startTime = getTime();
        this->main = functionProfile("Main", nullptr);

        this->main.functionList.push_back("Profiler");
        this->main.childProfileMap["Profiler"] = functionProfile("Profiler", &main);
        this->profilerUpdater = &main.childProfileMap.at("Profiler");

        this->currentProfile = &this->main;
    }

    void Profiler::end()
    {
        this->endTime = getTime();
    }

    void Profiler::logChildProfiles(functionProfile &profile, int depth)
    {
        for (std::string &s : profile.functionList)
        {
            functionProfile &f = profile.childProfileMap.at(s);
            std::string indent;
            for (int i = 0; i < depth; i++)
            {
                indent.append("     ");
            }
            double average = (f.count == 0) ? 0 : (f.totalTime / f.count);
            log(
                indent +
                f.functionName + ": " +
                std::to_string(average * 1000) + "ms, " +
                std::to_string(f.count) + ", " +
                std::to_string(f.totalTime) + "s, " +
                std::to_string(int(round((f.totalTime / profile.totalTime) * 100))) + "%");
            logChildProfiles(f, depth + 1);
        }
    }

    void Profiler::logProfilerData()
    {
        double totalRunTime = (this->endTime) - (this->startTime);
        log("Total Run time: " + std::to_string(totalRunTime) + "s");
        this->main.totalTime = totalRunTime;
        if (main.childProfileMap.size() > 0)
        {
            log("Profiler Data: Average time, Count, Total time, Percent");
            logChildProfiles(main, 1);
        }
        log("");
    }

    double Profiler::getTotalTime()
    {
        return this->endTime - this->startTime;
    }
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

            constexpr size_t MAX_WORDS = 2000000;
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