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

    //returns time in seconds
    double getTime(){
        return(std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::system_clock::now()).time_since_epoch()).count()*NANO_TO_SEC);
    }

    //class used to keep track of the time and number of calls a function has
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

        //updates the function profile
        //@param p points to the function profile to be set as the current function
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
    
    //Class to drive the function profiles and log profiler data
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

        //appends a string with line break to log.txt
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

        //updates or creates a function profile depending on what the current function profile is
        //@param start true if the starting a new function or false if ending the function
        void updateProfile(const std::string& functionName, bool start){
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

        //updates function profile to start
        //@param ignore profile is only updated if true, used to skip unnecessary calls in recusive functions 
        void profileStart(const std::string& functionName, bool ignore=false){
            if(!ignore){
                updateProfile(functionName, true);
            }
        }

        //updates function profile to end
        //@param ignore profile is only updated if true, used to skip unnecessary calls in recusive functions 
        void profileEnd(const std::string& functionName, bool ignore=false){
            if(!ignore){
                updateProfile(functionName, false);
            }
        }

        //sets the start time to the current time and initializes Main and Profiler funciton profiles
        void start(){
            this->startTime = getTime();
            this->main = functionProfile("Main", nullptr);
            
            this->main.functionList.push_back("Profiler");
            this->main.childProfileMap["Profiler"] = functionProfile("Profiler", &main);
            this->profilerUpdater = &main.childProfileMap.at("Profiler");

            this->currentProfile = &this->main;
            //this->profileMap = std::map<std::string, functionProfile>();

        }
        //sets the end time to the current time
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
                double average = (f.count == 0) ? 0: (f.totalTime/f.count);
                log(
                    indent +
                    f.functionName + ": " + 
                    std::to_string(average*1000) + "ms, " + 
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
                logChildProfiles(main, 1);
            }
            log("");
        }
    };
}