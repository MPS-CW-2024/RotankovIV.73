#include "zone_control.h"
#include "lcd.h"
#include "uart.h"
#include <avr/io.h>
#include <util/delay.h>

#define FLOW_CHECK_MS 10000
#define LEAK_THRESHOLD 20    // 20% отклонение
#define SOUND_PORT PORTE
// Пин вывода звукового сигнала
#define SOUND_PIN PE0
#define DELAY_MS_VAL 1500

static uint8_t leakStates = 0;

void playErrorTone() {
    for (int i = 0; i < DELAY_MS_VAL; i++) {
        SOUND_PORT |= (1 << SOUND_PIN);
        _delay_us(DELAY_MS_VAL / 2);
        SOUND_PORT &= ~(1 << SOUND_PIN);
        _delay_us(DELAY_MS_VAL / 2);
    }
}

void checkLeaks(Zone* zones) {
    // Читаем состояния кнопок утечки (активный уровень - низкий)
    uint8_t currentPB3 = (PINB & (1 << PB3)) == 0;
    uint8_t currentPB4 = (PINB & (1 << PB4)) == 0;
    
    // Проверяем каждую зону
    for(uint8_t i = 0; i < NUM_ZONES; i++) {
        uint8_t hasLeak = (i == 0) ? currentPB3 : currentPB4;
        
        // Обрабатываем утечку только если зона активна
        if(zones[i].isActive) {
            if(hasLeak) {
                if(!(leakStates & (1 << i))) {
                    leakStates |= (1 << i);
                    PORTD |= (1 << PD7);    // LED
                    playErrorTone();
                    char buf[16];
                    sprintf(buf, "Leak in Zone %d!", i + 1);
                    displayError(buf);
                }
            } else {
                leakStates &= ~(1 << i);
                if(!leakStates) {
                    PORTD &= ~(1 << PD7);
                    PORTE &= ~(1 << PE0);
                }
            }
        } else {
            // Если зона неактивна, сбрасываем её состояние утечки
            leakStates &= ~(1 << i);
            if(!leakStates) {
                PORTD &= ~(1 << PD7);
                PORTE &= ~(1 << PE0);
            }
        }
    }
}