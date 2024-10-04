#include <cmath>
#include <vector>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <map>
#include <string>

#include <iostream>
#include <conio.h>

namespace Profiler{

    const double NANO_TO_SEC = 1.0/1000000000;
    double getTime(){
        //returns time in seconds
        return(std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::system_clock::now()).time_since_epoch()).count()*NANO_TO_SEC);
    }

    class functionProfile{
        public:
        std::string functionName = "";
        std::map<std::string, functionProfile> childProfileMap;
        std::vector<std::string> functionList;
        functionProfile* parent;
        double startTime = 0;
        int count = 0;
        double totalTime = 0;

        functionProfile(){}

        functionProfile(const std::string& name, functionProfile* parent){
            this->functionName = name;
            this->parent = parent;
            this->childProfileMap = std::map<std::string, functionProfile>();
            this->functionList = std::vector<std::string>();
        }

        void update(double time, functionProfile*& p){
            if(this->startTime == 0){
                this->startTime = getTime();
                p = this;
            }else{
                this->totalTime += time-this->startTime;
                this->count = this->count + 1;
                this->startTime = 0;
                p = this->parent;
            }
        }
    };

    class Profiler{
        public:
        functionProfile* profilerUpdater;
        functionProfile main;
        functionProfile* currentProfile;
        std::string logDirectory = "";
        double startTime;
        double endTime;

        Profiler(){
            this->start();
        }

        void log(const std::string& message){
            std::ofstream logFile;
            if(this->logDirectory==""){
                logFile.open("log.txt",std::ios::app);
            }else{
                logFile.open(this->logDirectory + "\\log.txt",std::ios::app);
            }
            logFile << std::fixed << std::setprecision(9);
            logFile << message << "\n";
            logFile.close();
        }

        void updateProfiler(const std::string& functionName, bool start){
            double t = getTime();

            if(currentProfile->functionName == functionName && !start){
                currentProfile->update(t, currentProfile);
            }else{
                try{
                    functionProfile* p = &this->currentProfile->childProfileMap.at(functionName);
                    p->update(t, currentProfile);
                }catch(...){
                    this->currentProfile->functionList.push_back(functionName);                    
                    this->currentProfile->childProfileMap[functionName] = *new functionProfile(functionName, currentProfile);
                    this->currentProfile->childProfileMap.at(functionName).update(t, currentProfile);
                }
            }
            this->profilerUpdater->count++;
            this->profilerUpdater->totalTime += getTime()-t;
        }

        void startProfile(const std::string& functionName, bool ignore=false){
            if(!ignore){
                updateProfiler(functionName, true);
            }
        }

        void endProfile(const std::string& functionName, bool ignore=false){
            if(!ignore){
                updateProfiler(functionName, false);
            }
        }

        void start(){
            this->startTime = getTime();
            this->main = functionProfile("Main", nullptr);
            
            this->main.functionList.push_back("Profiler");
            this->main.childProfileMap["Profiler"] = functionProfile("Profiler", &main);
            this->profilerUpdater = &main.childProfileMap.at("Profiler");

            this->currentProfile = &this->main;
            //this->profileMap = std::map<std::string, functionProfile>();

        }
        void end(){
            this->endTime = getTime();
        }

        void logChildProfiles(functionProfile& profile, int depth){
            for(std::string& s: profile.functionList){
                functionProfile& f = profile.childProfileMap.at(s);
                std::string indent;
                for(int i = 0; i < depth; i++){
                    indent.append("     ");
                }
                log(
                    indent +
                    f.functionName + ": " + 
                    std::to_string((f.totalTime/f.count)*1000) + "ms, " + 
                    std::to_string(f.count) + ", " + 
                    std::to_string(f.totalTime) + "s, " +
                    std::to_string(int(round((f.totalTime/profile.totalTime)*100))) + "%"
                );
                logChildProfiles(f, depth+1);
            }
        }

        void logProfilerData(){
            double totalRunTime = (this->endTime)-(this->startTime);
            log("Total Run time: " + std::to_string(totalRunTime) + "s");
            this->main.totalTime = totalRunTime;
            if(main.childProfileMap.size() > 0){
                log("Profiler Data: Average time, Count, Total time, Percent");
                /*
                log(
                    this->profilerUpdater->functionName + ": " + 
                    std::to_string((this->profilerUpdater->totalTime/this->profilerUpdater->count)*1000) + "ms, " + 
                    std::to_string(this->profilerUpdater->count) + ", " + 
                    std::to_string(this->profilerUpdater->totalTime) + "s, " +
                    std::to_string(int(round((this->profilerUpdater->totalTime/totalRunTime)*100))) + "%"
                );
                for(std::string& s: this->functionList){
                    functionProfile& f = this->profileMap.at(s);
                    std::string indent;
                    for(int i = 0; i < f.depth; i++){
                        indent.append("     ");
                    }
                    log(
                        indent +
                        f.functionName + ": " + 
                        std::to_string((f.totalTime/f.count)*1000) + "ms, " + 
                        std::to_string(f.count) + ", " + 
                        std::to_string(f.totalTime) + "s, " +
                        std::to_string(int(round((f.totalTime/totalRunTime)*100))) + "%"
                    );
                }
                */
               logChildProfiles(main, 1);
            }
            log("");
        }
    };
}