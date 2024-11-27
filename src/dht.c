#include "dht.h"
#include <avr/io.h>
#include <util/delay.h>

static uint8_t dhtBits[5];
static uint8_t sensorError[2] = {0, 0};

void initDht(void) {
    // Configure DHT pins as inputs initially
    DDRD &= ~((1 << PD3) | (1 << PD4));
    // Enable pullups
    PORTD |= (1 << PD3) | (1 << PD4);
}

static uint8_t readDht(uint8_t pin) {
    uint8_t retries = 0;
    uint8_t pinMask = (1 << pin);
    uint8_t i, j;
    
    // Reset error flags at start of reading
    for(i = 0; i < 5; i++) {
        dhtBits[i] = 0;
    }
    
    // Initial signal: pull low for 1ms
    DDRD |= pinMask;           // Output
    PORTD &= ~pinMask;         // Low
    _delay_ms(20);             // DHT22 needs at least 1ms
    
    // Release line and wait for response
    PORTD |= pinMask;          // High
    DDRD &= ~pinMask;          // Input
    _delay_us(40);
    
    // Check start condition
    if(PIND & pinMask) return 0;
    _delay_us(80);
    if(!(PIND & pinMask)) return 0;
    _delay_us(80);

    // Read 40 bits
    for(i = 0; i < 40; i++) {
        // Wait for falling edge
        retries = 0;
        while(PIND & pinMask) {
            _delay_us(1);
            if(++retries > 100) return 0;
        }
        
        // Measure time until rising edge
        retries = 0;
        while(!(PIND & pinMask)) {
            _delay_us(1);
            if(++retries > 100) return 0;
        }
        
        // Wait 40us and check bit value
        _delay_us(30);
        j = i/8;
        dhtBits[j] <<= 1;
        if(PIND & pinMask) {
            dhtBits[j] |= 1;
        }
    }
    
    // Verify checksum
    uint8_t checksum = dhtBits[0] + dhtBits[1] + dhtBits[2] + dhtBits[3];
    if(checksum != dhtBits[4]) return 0;
    
    return 1;
}

void processDhtData(Zone* zone, uint8_t sensorNum) {
    static uint16_t readDelay = 0;
    
    if(readDelay < 2000) { // Wait 2 seconds between readings
        readDelay++;
        return;
    }
    readDelay = 0;
    
    uint8_t pin = sensorNum ? PD4 : PD3;
    
    if(readDht(pin)) {
        // DHT22 returns humidity as: integral*10 + decimal
        uint16_t humidity = (dhtBits[0] << 8) + dhtBits[1];
        zone->humidity = humidity / 10;
        sensorError[sensorNum] = 0;
    } else {
        if(++sensorError[sensorNum] >= 3) {
            // After 3 consecutive errors, mark sensor as failed
            zone->humidity = 0;
        }
    }
}