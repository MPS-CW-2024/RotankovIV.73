#pragma once
#include <stdint.h>
#include <stdbool.h>

#define NUM_ZONES 2

typedef struct {
    uint8_t humidity;
    uint8_t targetHumidity;
    uint16_t wateringTime;
    uint16_t timeRemaining;
    uint16_t flowCount;
    uint8_t flowRate;
    uint8_t startHour;
    uint8_t startMinute;
    uint8_t isActive;
    uint8_t isManual;
} Zone;

typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t isSettingTime;    // Flag for time setting mode
} SystemTime;

void initZones(Zone* zones);
void updateZone(Zone* zone);
void adjustParameter(Zone* zone, uint8_t param, int8_t change);
void toggleManual(Zone* zone);
void checkLeaks(Zone* zones);

// new functions for global time setting
void incrementTime(SystemTime* time);
void adjustTime(SystemTime* time, int8_t minuteChange);
void setTime(SystemTime* time, uint8_t hours, uint8_t minutes, uint8_t seconds);

// Parameters
#define PARAM_HUMIDITY 0
#define PARAM_TIME     1 
#define PARAM_FLOW     2
#define PARAM_SCHEDULE 3