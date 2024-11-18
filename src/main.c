#include <avr/io.h>
#include <avr/interrupt.h>
#include "zone.h"
#include "time.h"
#include "lcd.h"
#include "uart.h"
#include "dht.h"

static Zone zones[NUM_ZONES];
static SystemTime systemTime;
static uint8_t currentZone = 0;
static uint8_t selectedParam = 0;

static volatile uint8_t lastButtonState = 0;
static volatile uint8_t lastTimeButtonState = 0;
static volatile uint16_t debounceTime = 0;

ISR(INT0_vect) { zones[0].flowCount++; }
ISR(INT1_vect) { zones[1].flowCount++; }

ISR(TIMER0_COMP_vect) {
    static uint16_t msCounter = 0;
    
    if(++msCounter >= 1000) {
        msCounter = 0;
        incrementTime(&systemTime);
        
        for(uint8_t i = 0; i < NUM_ZONES; i++) {
            updateZone(&zones[i], &systemTime);
        }
    }
    
    if(debounceTime > 0) debounceTime--;
}

static void initPorts(void) {
    // Setup zone buttons with pullups
    DDRC &= ~((1<<PC3)|(1<<PC4)|(1<<PC5)|(1<<PC6)|(1<<PC7));
    PORTC |= (1<<PC3)|(1<<PC4)|(1<<PC5)|(1<<PC6)|(1<<PC7);
    
    // Setup Time button on PD6 with pullup
    DDRD &= ~(1<<PD6);
    PORTD |= (1<<PD6);
    
    // Outputs
    DDRB |= (1<<PB0)|(1<<PB1); // Pump LEDs
    DDRD |= (1<<PD7);          // Leak LED
    DDRE |= (1<<PE0);          // Buzzer
}

static void handleButtons(void) {
    if(debounceTime > 0) return;

    uint8_t buttons = ~PINC & 0xF8; // Zone buttons (PC3-PC7)
    uint8_t timeButton = ~PIND & (1<<PD6);
    
    if(buttons != lastButtonState || timeButton != lastTimeButtonState) {
        debounceTime = 20;
        lastButtonState = buttons;
        lastTimeButtonState = timeButton;
        
        if(timeButton) {
            toggleTimeSettings(&systemTime);
            if(systemTime.isSettingTime) {
                updateTimeDisplay(&systemTime);
            }
        }
        else if(systemTime.isSettingTime) {
            if(buttons & (1<<PC5)) adjustTime(&systemTime, 5);  // V+
            if(buttons & (1<<PC6)) adjustTime(&systemTime, -5); // V-
            updateTimeDisplay(&systemTime);
        }
        else {
            if(buttons & (1<<PC3)) currentZone = (currentZone + 1) % NUM_ZONES;
            if(buttons & (1<<PC4)) selectedParam = (selectedParam + 1) % 4;
            if(buttons & (1<<PC5)) adjustParameter(&zones[currentZone], selectedParam, 1);
            if(buttons & (1<<PC6)) adjustParameter(&zones[currentZone], selectedParam, -1);
            if(buttons & (1<<PC7)) toggleManual(&zones[currentZone]);
            
            updateDisplay(zones, currentZone, selectedParam);
        }
    }
}

int main(void) {
    initPorts();
    initZones(zones);
    initSystemTime(&systemTime);
    initLcd();
    initUart();
    initDht();
    
    OCR0 = 57;
    TCCR0 = (1<<WGM01)|(1<<CS01)|(1<<CS00);
    TIMSK |= (1<<OCIE0);
    
    MCUCR |= (1<<ISC01)|(1<<ISC11);
    GICR |= (1<<INT0)|(1<<INT1);
    
    sei();
    
    while(1) {
        handleButtons();
        processDhtData(&zones[0], 0);
        processDhtData(&zones[1], 1);
        checkLeaks(zones);
        
        if(!systemTime.isSettingTime) {
            updateDisplay(zones, currentZone, selectedParam);
        }
    }
    
    return 0;
}