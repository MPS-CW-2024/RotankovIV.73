#include "zone_control.h"
#include "lcd.h"
#include <avr/io.h>

#define FLOW_CHECK_MS 1000
#define LEAK_THRESHOLD 20    // 20% отклонение

static uint8_t leakStates = 0;

void handleZoneAlarm(uint8_t zoneIdx, uint8_t hasLeak) {
    if(hasLeak) {
        if(!(leakStates & (1 << zoneIdx))) {
            leakStates |= (1 << zoneIdx);
            PORTD |= (1 << PD7);    // LED
            PORTE |= (1 << PE0);    // Buzzer
            displayError("Flow Error!");
        }
    } else {
        leakStates &= ~(1 << zoneIdx);
        if(!leakStates) {
            PORTD &= ~(1 << PD7);
            PORTE &= ~(1 << PE0);
        }
    }
}

void checkLeaks(Zone* zones) {
    static uint16_t msCounter = 0;
    static uint8_t lastPB3 = 1;
    static uint8_t lastPB4 = 1;
    
    // Читаем пины расходомеров
    uint8_t currentPB3 = (PINB & (1 << PB3)) != 0;
    uint8_t currentPB4 = (PINB & (1 << PB4)) != 0;
    
    // Считаем импульсы по спадающему фронту
    if(zones[0].isActive && !currentPB3 && lastPB3) {
        zones[0].flowCount++;
    }
    if(zones[1].isActive && !currentPB4 && lastPB4) {
        zones[1].flowCount++;
    }
    
    lastPB3 = currentPB3;
    lastPB4 = currentPB4;
    
    if(++msCounter >= FLOW_CHECK_MS) {
        msCounter = 0;
        
        for(uint8_t i = 0; i < NUM_ZONES; i++) {
            if(zones[i].isActive) {
                // Сравниваем напрямую количество импульсов
                uint8_t expected = zones[i].flowRate; // Ожидаемая частота
                uint8_t actual = zones[i].flowCount;  // Фактическая частота
                uint8_t hasLeak = 0;
                
                // Проверяем выход за пределы ±20%
                uint8_t minFlow = (expected * 8) / 10;  // 80%
                uint8_t maxFlow = (expected * 12) / 10; // 120%
                
                if(actual < minFlow || actual > maxFlow) {
                    hasLeak = 1;
                }
                
                handleZoneAlarm(i, hasLeak);
            } else {
                handleZoneAlarm(i, 0);
            }
            
            zones[i].flowCount = 0;
        }
    }
}