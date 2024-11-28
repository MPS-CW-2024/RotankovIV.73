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

// Флаг защиты от обновлений по таймеру 
extern volatile uint8_t zoneUpdateDisabled;

void initZones(Zone* zones);
void updateZone(Zone* zone, uint8_t index, SystemTime* time);
// void setZoneActive(Zone* zone, uint8_t index, uint8_t active);
void adjustParameter(Zone* zone, uint8_t param, int8_t change);
void toggleManual(Zone* zone, uint8_t index);



// Parameters
#define PARAM_HUMIDITY 0
#define PARAM_TIME     1 
#define PARAM_FLOW     2
#define PARAM_SCHEDULE 3