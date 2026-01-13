#include <iostream>
#include <chrono>
#include <thread>

class Timer {
private:
    unsigned int days;
    unsigned int hours;
    unsigned int minutes;
    unsigned int seconds;

public:
    Timer(unsigned int d = 0, unsigned int h = 0, unsigned int m = 0, unsigned int s = 0)
        : days(d), hours(h), minutes(m), seconds(s) {}

    void setDays(unsigned int d) {
        days = d;
    }

    void setHours(unsigned int h) {
        hours = h;
    }

    void setMinutes(unsigned int m) {
        minutes = m;
    }

    void setSeconds(unsigned int s) {
        seconds = s;
    }

    unsigned int getDays() const { return days; }
    unsigned int getHours() const { return hours; }
    unsigned int getMinutes() const { return minutes; }
    unsigned int getSeconds() const { return seconds; }

    unsigned int getTotalSeconds() const {
        return seconds + (minutes * 60) + (hours * 3600) + (days * 86400);
    }

    void start() {
        unsigned int totalSeconds = getTotalSeconds();

        if (totalSeconds == 0) {
            std::cout << "Timer is not set.\n";
            return;
        }

        std::cout << "Timer started for: " << days << " days, "
                  << hours << " hours, " << minutes << " minutes, "
                  << seconds << " seconds\n";

        while (totalSeconds > 0) {
            std::cout << "Time remaining: " << totalSeconds << " seconds\r" << std::flush;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            totalSeconds--;
        }

        std::cout << "\nTimer finished!\n";
    }
};

// int main() {
//     // Example usage - create and set timer values directly
//     Timer timer;
    
//     // Set timer values directly without interaction
//     timer.setDays(0);
//     timer.setHours(0);
//     timer.setMinutes(0);
//     timer.setSeconds(5);
    
//     // Start the timer
//     timer.start();
    
//     return 0;
// }