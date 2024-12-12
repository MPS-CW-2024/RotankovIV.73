#include "dht.h"
#include <avr/io.h>
#include <util/delay.h>

static uint8_t dhtBits[5];
static uint8_t sensorError[2] = {0, 0};

void initDht(void) {
    // Настройка соотв пинов на вход
    DDRD &= ~((1 << PD3) | (1 << PD4));
    // Включаем подтягивающие резисторы
    PORTD |= (1 << PD3) | (1 << PD4);
}

static uint8_t readDht(uint8_t pin) {
    uint8_t retries = 0;
    uint8_t pinMask = (1 << pin);
    uint8_t i, j;
    
    // Ресетим флаги
    for(i = 0; i < 5; i++) {
        dhtBits[i] = 0;
    }
    
    DDRD |= pinMask;           // Выход
    PORTD &= ~pinMask;         // Low
    _delay_ms(20);             // DHT22 требуется как минимум 10мс
    
    
    PORTD |= pinMask;          // High
    DDRD &= ~pinMask;          // Input
    _delay_us(40);
    
    // Проверяем начальные условия
    if(PIND & pinMask) return 0;
    _delay_us(80);
    if(!(PIND & pinMask)) return 0;
    _delay_us(80);

    // Считываем 40 бит
    for(i = 0; i < 40; i++) {
        // Ждем отрицательного фронта
        retries = 0;
        while(PIND & pinMask) {
            _delay_us(1);
            if(++retries > 100) return 0;
        }
        
        // Измеряем время до положительного фронта 
        retries = 0;
        while(!(PIND & pinMask)) {
            _delay_us(1);
            if(++retries > 100) return 0;
        }
        
        // Wait 40us и проверяем значение бита
        _delay_us(30);
        j = i/8;
        dhtBits[j] <<= 1;
        if(PIND & pinMask) {
            dhtBits[j] |= 1;
        }
    }
    
    // Проверка checksum
    uint8_t checksum = dhtBits[0] + dhtBits[1] + dhtBits[2] + dhtBits[3];
    if(checksum != dhtBits[4]) return 0;
    
    return 1;
}

void processDhtData(Zone* zone, uint8_t sensorNum) {
    static uint16_t readDelay = 0;
    
    if(readDelay < 2000) { // Ждем по 2 секунды между чтением
        readDelay++;
        return;
    }
    readDelay = 0;
    
    uint8_t pin = sensorNum ? PD4 : PD3;
    
    if(readDht(pin)) {
        // DHT22 возвращает влажность в формате: integral*10 + decimal
        uint16_t humidity = (dhtBits[0] << 8) + dhtBits[1];
        zone->humidity = humidity / 10;
        sensorError[sensorNum] = 0;
    } else {
        if(++sensorError[sensorNum] >= 3) {
            zone->humidity = 0;
        }
    }
}