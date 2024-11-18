#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "time.h"

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

void initZones(Zone* zones);
void updateZone(Zone* zone, SystemTime* time);
void adjustParameter(Zone* zone, uint8_t param, int8_t change);
void toggleManual(Zone* zone);



// Parameters
#define PARAM_HUMIDITY 0
#define PARAM_TIME     1 
#define PARAM_FLOW     2
#define PARAM_SCHEDULE 3