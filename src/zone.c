#include "zone.h"
#include <avr/io.h>
#include "time.h"

void initZones(Zone* zones) {
    for(uint8_t i = 0; i < NUM_ZONES; i++) {
        zones[i].humidity = 0;
        zones[i].targetHumidity = 60;
        zones[i].wateringTime = 30;
        zones[i].timeRemaining = 0;
        zones[i].flowCount = 0;
        zones[i].flowRate = 2;     // 2 L/min default
        zones[i].startHour = 0;
        zones[i].startMinute = 0;
        zones[i].isActive = 0;
        zones[i].isManual = 0;
    }
}

void updateZone(Zone* zone, uint8_t index, SystemTime* time) {
    if(!zone->isActive && !zone->isManual) {
        if(time->hours == zone->startHour && 
           time->minutes == zone->startMinute &&
           time->seconds == 0) {
            zone->isActive = 1;
            zone->timeRemaining = zone->wateringTime * 60;
            PORTB |= (1 << index);
        }
    }
    
    if(zone->isActive && !zone->isManual) {
        if(zone->timeRemaining > 0) {
            zone->timeRemaining--;
        }
        if(zone->timeRemaining == 0 || 
           zone->humidity >= zone->targetHumidity) {
            zone->isActive = 0;
            PORTB &= ~(1 << index);
        }
    }
}

void toggleManual(Zone* zone, uint8_t index) {
    zone->isManual = !zone->isManual;
    zone->isActive = zone->isManual;
    
    // Обновляем состояние помпы
    if(zone->isActive) {
        PORTB |= (1 << index);  // Включаем LED помпы
    } else {
        PORTB &= ~(1 << index); // Выключаем LED помпы
        zone->timeRemaining = 0;
    }
    
    // Отправляем статус по UART
    sendZoneData(index);
}

void adjustParameter(Zone* zone, uint8_t param, int8_t change) {
    switch(param) {
        case PARAM_SCHEDULE: {
            int16_t totalMinutes = zone->startHour * 60 + zone->startMinute;
            totalMinutes += change * 5;
            
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
            if(change > 0 && zone->wateringTime < 120) {
                zone->wateringTime += 5;
            } else if(change < 0 && zone->wateringTime > 5) {
                zone->wateringTime -= 5;
            }
            break;
            
        case PARAM_FLOW:
            if(change > 0 && zone->flowRate < 20) {
                zone->flowRate++;
            } else if(change < 0 && zone->flowRate > 1) {
                zone->flowRate--;
            }
            break;
    }
}