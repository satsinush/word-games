#include <chrono>
#include <string>

#include <iostream>
#include <conio.h>

namespace Status{
    class Printer{
        private:
        const double NANO_TO_SEC = 1.0/1000000000;
        
        double lastUpdate;
        int lastMessageSize;

        //returns time in seconds
        double getTime(){
            return(std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::system_clock::now()).time_since_epoch()).count()*NANO_TO_SEC);
        }

        public:
        Printer(){
            lastUpdate = getTime();
        }

        void printUpdate(std::string message, double delay){
            double time = getTime();
            if(time-lastUpdate > delay){
                lastUpdate = time;
                std::cout << "\r" << std::string(lastMessageSize+1,' ') << "\r";
                std::cout << message;
                lastMessageSize = message.size();
            }   
        }
    };
}