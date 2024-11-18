#include "dht.h"
#include <avr/io.h>
#include <util/delay.h>

static uint8_t dhtBits[5];

static uint8_t readDht(uint8_t pin) {
    uint8_t result = 0;
    uint8_t pinMask = (1 << pin);
    
    DDRD |= pinMask;
    PORTD &= ~pinMask;
    _delay_ms(1);
    
    PORTD |= pinMask;
    DDRD &= ~pinMask;
    _delay_us(40);
    
    if((PIND & pinMask)) {
        return 0;
    }
    _delay_us(80);
    
    if(!(PIND & pinMask)) {
        return 0;
    }
    _delay_us(80);
    
    // Читаем 40 бит
    for(uint8_t i = 0; i < 40; i++) {
        // Ждем отрицательного фронта
        while((PIND & pinMask));
        // Ждем положительного фронта
        while(!(PIND & pinMask));
        // Ждем 40 мс, и проверяем значение на пине
        _delay_us(30);
        if(PIND & pinMask) {
            result |= (1 << (7 - (i % 8)));
        }
        if((i % 8) == 7) {
            dhtBits[i/8] = result;
            result = 0;
        }
    }
    
    // проверяем checksum
    if(dhtBits[4] == ((dhtBits[0] + dhtBits[1] + dhtBits[2] + dhtBits[3]) & 0xFF)) {
        return 1;
    }
    return 0;
}

void initDht(void) {
    // Настраиваем пины для обработки DHT на вход
    DDRD &= ~((1 << PD3) | (1 << PD4));
    // Включаем подтягивающие резисторы
    PORTD |= (1 << PD3) | (1 << PD4);
}

void processDhtData(Zone* zone, uint8_t sensorNum) {
    static uint8_t retryCount = 0;
    static uint16_t readDelay = 0;
    
    if(readDelay < 2000) {
        readDelay++;
        return;
    }
    readDelay = 0;
    
    if(readDht(sensorNum ? PD4 : PD3)) {
        zone->humidity = dhtBits[0];
        retryCount = 0;
    } else if(retryCount < 3) {
        retryCount++;
    }
}