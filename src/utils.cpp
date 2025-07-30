#include "utils.hpp"
#include <chrono>
#include <string>
#include <regex>
#include <iostream>
#include <conio.h>
#include <cmath>
#include <vector>
#include <fstream>
#include <map>
#include <iomanip>

namespace Utils
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