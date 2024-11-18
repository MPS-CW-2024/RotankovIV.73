#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "zone.h"
#include "zone_control.h"
#include "lcd.h"
#include "uart.h"
#include "dht.h"

static Zone zones[NUM_ZONES];
static uint8_t currentMenu = 0;
static uint8_t selectedParam = 0;

// проверка на дребезг
static volatile uint8_t lastButtonState = 0;
static volatile uint16_t debounceTime = 0;

ISR(INT0_vect) {
    zones[0].flowCount++;
}

ISR(INT1_vect) {
    zones[1].flowCount++; 
}

ISR(TIMER0_COMP_vect) {
    static uint16_t msCounter = 0;
    
    if(++msCounter >= 1000) {
        msCounter = 0;
        for(uint8_t i = 0; i < NUM_ZONES; i++) {
            updateZone(&zones[i]);
        }
        updateZoneLeds(zones);
    }
    
    if(debounceTime > 0) {
        debounceTime--;
    }
}

static void initPorts(void) {
    DDRC &= ~((1<<PC3)|(1<<PC4)|(1<<PC5)|(1<<PC6)|(1<<PC7));
    PORTC |= (1<<PC3)|(1<<PC4)|(1<<PC5)|(1<<PC6)|(1<<PC7);
    
    DDRB |= (1<<PB0)|(1<<PB1)|(1<<PB2);
    DDRD |= (1<<PD7);
    DDRE |= (1<<PE0);
}

static void handleButtons(void) {
    if(debounceTime > 0) return;
    
    uint8_t buttons = ~PINC & 0xF8;
    if(buttons != lastButtonState) {
        debounceTime = 20;
        lastButtonState = buttons;
        
        for(uint8_t i = 3; i <= 7; i++) {
            if(buttons & (1 << i)) {
                if(i == 3) currentMenu = (currentMenu + 1) % NUM_ZONES;
                else if(i == 4) selectedParam = (selectedParam + 1) % 3;
                else if(i == 5) adjustParameter(&zones[currentMenu], selectedParam, 1);
                else if(i == 6) adjustParameter(&zones[currentMenu], selectedParam, -1);
                else if(i == 7) toggleManual(&zones[currentMenu]);
                
                updateDisplay(zones, currentMenu, selectedParam);
                break;
            }
        }
    }
}

int main(void) {
    initPorts();
    initZones(zones);
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
    }
    
    return 0;
}