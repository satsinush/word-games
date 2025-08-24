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
    const double NANO_TO_SEC = 1.0 / 1000000000;

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

    // Loads words from words.bin if available, otherwise from .txt files in data directory and saves to words.bin.
    std::vector<Word> loadWords()
    {
        std::filesystem::path data_dir = std::filesystem::current_path() / "data";
        std::filesystem::path word_lists_dir = std::filesystem::current_path() / "word_lists";
        std::vector<Word> allWordsVec;
        std::ifstream in(data_dir / "words.bin", std::ios::binary);
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
                    in.read(reinterpret_cast<char *>(&allWordsVec[i].uniqueLetters), sizeof(allWordsVec[i].uniqueLetters));
                    in.read(reinterpret_cast<char *>(&allWordsVec[i].order), sizeof(allWordsVec[i].order));
                    in.read(reinterpret_cast<char *>(&allWordsVec[i].count), sizeof(allWordsVec[i].count));
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

        if (!loadedFromBin)
        {
            std::vector<std::string> wordFiles;
            for (const auto &entry : std::filesystem::directory_iterator(word_lists_dir))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".txt")
                {
                    wordFiles.push_back(entry.path().filename().string());
                }
            }
            std::sort(wordFiles.begin(), wordFiles.end());

            std::set<Word> allWordsSet;
            int order = 0;

            for (const auto &fname : wordFiles)
            {
                std::ifstream file(word_lists_dir / fname);
                if (!file.is_open())
                {
                    std::cerr << "Error: Could not open " << fname << ". Please ensure it's in a 'data' sub-directory.\n";
                    continue;
                }
                std::string line;
                while (std::getline(file, line))
                {
                    std::istringstream iss(line);
                    std::string word;
                    while (iss >> word)
                    {
                        word = trimToLower(word);

                        if (word.empty() || std::any_of(word.begin(), word.end(), [](unsigned char c)
                                                        { return !std::isalpha(c); }))
                        {
                            continue;
                        }

                        int uniqueLetters = std::set<char>(word.begin(), word.end()).size();

                        // Calculate letter count array
                        std::array<uint8_t, 26> letterCount = {0};
                        for (char c : word)
                        {
                            letterCount[c - 'a']++;
                        }

                        auto result = allWordsSet.insert({word, order, 1, uniqueLetters, letterCount});
                        if (!result.second)
                        {
                            auto it = result.first;
                            Word updatedWord = *it;
                            allWordsSet.erase(it);
                            updatedWord.count += 1;
                            allWordsSet.insert(updatedWord);
                        }
                    }
                }
                file.close();
                order++;
            }
            allWordsVec.assign(allWordsSet.begin(), allWordsSet.end());

            // Save to binary for next time
            std::ofstream out(data_dir / "words.bin", std::ios::binary);
            size_t n = allWordsVec.size();
            out.write(reinterpret_cast<const char *>(&n), sizeof(n));
            for (const auto &w : allWordsVec)
            {
                size_t len = w.wordString.size();
                out.write(reinterpret_cast<const char *>(&len), sizeof(len));
                out.write(w.wordString.data(), len);
                out.write(reinterpret_cast<const char *>(&w.uniqueLetters), sizeof(w.uniqueLetters));
                out.write(reinterpret_cast<const char *>(&w.order), sizeof(w.order));
                out.write(reinterpret_cast<const char *>(&w.count), sizeof(w.count));
                out.write(reinterpret_cast<const char *>(&w.letterCount), sizeof(w.letterCount));
            }
            out.close();
        }
        return allWordsVec;
    }
} // namespace WordUtils