#pragma once
#include <stdint.h>

typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t isSettingTime;
} SystemTime;

void initSystemTime(SystemTime* time);
void incrementTime(SystemTime* time);
void adjustTime(SystemTime* time, int8_t minuteChange);
void setTime(SystemTime* time, uint8_t hours, uint8_t minutes, uint8_t seconds);
void toggleTimeSettings(SystemTime* time);