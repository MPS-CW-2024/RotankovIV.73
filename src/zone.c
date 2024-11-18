#include "zone.h"
#include "time.h"
#include <avr/io.h>

void initZones(Zone* zones) {
    for(uint8_t i = 0; i < NUM_ZONES; i++) {
        zones[i].humidity = 0;
        zones[i].targetHumidity = 60;
        zones[i].wateringTime = 30;    // Default 30 minutes
        zones[i].timeRemaining = 0;
        zones[i].flowCount = 0;
        zones[i].flowRate = 15;        // Expected 15Hz
        zones[i].startHour = 0;
        zones[i].startMinute = 0;
        zones[i].isActive = 0;
        zones[i].isManual = 0;
    }
}

void updateZone(Zone* zone, SystemTime* time) {
    // Check if it's time to start scheduled watering
    if(!zone->isActive && !zone->isManual) {
        if(time->hours == zone->startHour && 
           time->minutes == zone->startMinute &&
           time->seconds == 0) {
            zone->isActive = 1;
            zone->timeRemaining = zone->wateringTime * 60; // Convert to seconds
        }
    }
    
    // Process active zones
    if(zone->isActive) {
        if(!zone->isManual) {
            if(zone->timeRemaining > 0) {
                zone->timeRemaining--;
            }
            // Stop watering if time expired or humidity reached
            if(zone->timeRemaining == 0 || 
               zone->humidity >= zone->targetHumidity) {
                zone->isActive = 0;
            }
        }
    }
}

void adjustParameter(Zone* zone, uint8_t param, int8_t change) {
    switch(param) {
        case PARAM_SCHEDULE: {
            int16_t totalMinutes = zone->startHour * 60 + zone->startMinute;
            totalMinutes += change * 5; // 5 minute steps
            
            while(totalMinutes < 0) totalMinutes += 24 * 60;
            while(totalMinutes >= 24 * 60) totalMinutes -= 24 * 60;
            
            zone->startHour = totalMinutes / 60;
            zone->startMinute = totalMinutes % 60;
            break;
        }
        
        case PARAM_HUMIDITY:
            if(change > 0 && zone->targetHumidity < 95) {
                zone->targetHumidity += 5;
            } else if(change < 0 && zone->targetHumidity > 5) {
                zone->targetHumidity -= 5;
            }
            break;
            
        case PARAM_TIME:
            if(change > 0 && zone->wateringTime < 120) { // Max 2 hours
                zone->wateringTime += 5;
            } else if(change < 0 && zone->wateringTime > 5) {
                zone->wateringTime -= 5;
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