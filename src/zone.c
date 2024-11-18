#include "zone.h"
#include <avr/io.h>

void initZones(Zone* zones) {
    for(uint8_t i = 0; i < NUM_ZONES; i++) {
        zones[i].humidity = 0;
        zones[i].targetHumidity = 60;
        zones[i].wateringTime = 300;
        zones[i].timeRemaining = 0;
        zones[i].flowCount = 0;
        zones[i].flowRate = 15;
        zones[i].isActive = 0;
        zones[i].isManual = 0;
    }
}

void updateZone(Zone* zone) {
    if(zone->isActive) {
        if(!zone->isManual) {
            if(zone->timeRemaining > 0) {
                zone->timeRemaining--;
            }
            if(zone->timeRemaining == 0 || zone->humidity >= zone->targetHumidity) {
                zone->isActive = 0;
            }
        }
    }
}

void adjustParameter(Zone* zone, uint8_t param, int8_t change) {
    switch(param) {
        case PARAM_HUMIDITY:
            if(change > 0 && zone->targetHumidity < 95) {
                zone->targetHumidity += 5;
            } else if(change < 0 && zone->targetHumidity > 5) {
                zone->targetHumidity -= 5;
            }
            break;
            
        case PARAM_TIME:
            if(change > 0 && zone->wateringTime < 3600) {
                zone->wateringTime += 30;
            } else if(change < 0 && zone->wateringTime > 30) {
                zone->wateringTime -= 30;
            }
            break;
            
        case PARAM_FLOW:
            if(change > 0 && zone->flowRate < 30) {
                zone->flowRate++;
            } else if(change < 0 && zone->flowRate > 1) {
                zone->flowRate--;
            }
            break;
    }
}

void toggleManual(Zone* zone) {
    zone->isManual = !zone->isManual;
    zone->isActive = zone->isManual;
    if(!zone->isManual) {
        zone->timeRemaining = 0;
    }
}