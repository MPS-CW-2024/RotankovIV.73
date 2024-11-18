#include "zone_control.h"
#include <avr/io.h>

void updateZoneLeds(Zone* zones) {
    uint8_t pumpStatus = 0;
    for(uint8_t i = 0; i < NUM_ZONES; i++) {
        if(zones[i].isActive) {
            pumpStatus |= (1 << i);
        }
    }
    PORTB = (PORTB & 0xF8) | pumpStatus;
}

void handleZoneAlarm(uint8_t zoneIdx, uint8_t hasLeak) {
    if(hasLeak) {
        PORTD |= (1 << PD7);  // Leak LED
        PORTE |= (1 << PE0);  // Buzzer
    } else {
        PORTD &= ~(1 << PD7);
        PORTE &= ~(1 << PE0);
    }
}

void checkLeaks(Zone* zones) {
    static uint16_t lastCheck = 0;
    lastCheck++;
    
    if(lastCheck >= 100) {
        lastCheck = 0;
        
        for(uint8_t i = 0; i < NUM_ZONES; i++) {
            if(zones[i].isActive) {
                uint8_t actualRate = zones[i].flowCount;
                zones[i].flowCount = 0;
                
                uint8_t hasLeak = (actualRate > (zones[i].flowRate + 2) || 
                                 actualRate < (zones[i].flowRate - 2));
                handleZoneAlarm(i, hasLeak);
            }
        }
    }
}