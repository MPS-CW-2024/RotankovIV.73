#include "time.h"

void initSystemTime(SystemTime* time) {
    time->hours = 0;
    time->minutes = 0;
    time->seconds = 0;
    time->isSettingTime = 0;
}

void incrementTime(SystemTime* time) {
    time->seconds++;
    if(time->seconds >= 60) {
        time->seconds = 0;
        time->minutes++;
        
        if(time->minutes >= 60) {
            time->minutes = 0;
            time->hours++;
            
            if(time->hours >= 24) {
                time->hours = 0;
            }
        }
    }
}

void adjustTime(SystemTime* time, int8_t minuteChange) {
    if(!time->isSettingTime) return;
    
    int16_t totalMinutes = time->hours * 60 + time->minutes;
    totalMinutes += minuteChange;
    
    // Handle wrap-around
    while(totalMinutes < 0) totalMinutes += 24 * 60;
    while(totalMinutes >= 24 * 60) totalMinutes -= 24 * 60;
    
    time->hours = totalMinutes / 60;
    time->minutes = totalMinutes % 60;
}

void setTime(SystemTime* time, uint8_t hours, uint8_t minutes, uint8_t seconds) {
    if(hours < 24 && minutes < 60 && seconds < 60) {
        time->hours = hours;
        time->minutes = minutes;
        time->seconds = seconds;
    }
}

void toggleTimeSettings(SystemTime* time) {
    time->isSettingTime = !time->isSettingTime;
}