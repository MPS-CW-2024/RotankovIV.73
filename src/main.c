#include <avr/io.h>
#include <avr/interrupt.h>
#include "zone.h"
#include "time.h"
#include "lcd.h"
#include "uart.h"
#include "dht.h"
#include "zone_control.h"

// Глобальные переменные, доступные для других модулей
Zone zones[NUM_ZONES];
SystemTime systemTime;

// Локальные переменные
static uint8_t currentZone = 0;
static uint8_t selectedParam = 0;
static uint8_t displayNeedsUpdate = 1;

static volatile uint8_t lastButtonState = 0;
static volatile uint8_t lastTimeButtonState = 0;
static volatile uint16_t debounceTime = 0;

extern volatile uint8_t commandReady;

ISR(TIMER0_COMP_vect) {
    static uint16_t msCounter = 0;
    
    if(++msCounter >= 1000) {
        msCounter = 0;
        incrementTime(&systemTime);
        
        for(uint8_t i = 0; i < NUM_ZONES; i++) {
            updateZone(&zones[i], i, &systemTime);
        }
        
        if(systemTime.isSettingTime) {
            displayNeedsUpdate = 1;
        }
    }
    
    if(debounceTime > 0) {
        debounceTime--;
    }
}

static void initPorts(void) {
    // Кнопки зон
    DDRC &= ~((1<<PC3)|(1<<PC4)|(1<<PC5)|(1<<PC6)|(1<<PC7));
    PORTC |= (1<<PC3)|(1<<PC4)|(1<<PC5)|(1<<PC6)|(1<<PC7);
    
    // Настройка времени PD6
    DDRD &= ~(1<<PD6);
    PORTD |= (1<<PD6);
    
    // Детекция утечек
    DDRB &= ~((1<<PB3)|(1<<PB4));
    PORTB |= (1<<PB3)|(1<<PB4);
    
    // Выводы
    DDRB |= (1<<PB0)|(1<<PB1);     // Светодиоды помп
    PORTB &= ~((1<<PB0)|(1<<PB1));
    
    DDRD |= (1<<PD7);              // Светодиод утечки
    PORTD &= ~(1<<PD7);            
    
    DDRE |= (1<<PE0);              // Пьезоизлучатель
    PORTE &= ~(1<<PE0);
}

static void handleButtons(void) {
    if(debounceTime > 0) return;

    uint8_t buttons = ~PINC & 0xF8;
    // char buf[32];
    uint8_t timeButton = ~PIND & (1<<PD6);
    
    if(buttons != lastButtonState || timeButton != lastTimeButtonState) {
        debounceTime = 20;
        lastButtonState = buttons;
        lastTimeButtonState = timeButton;
        
        if(timeButton) {
            systemTime.isSettingTime = !systemTime.isSettingTime;
            displayNeedsUpdate = 1;
            if(systemTime.isSettingTime) {
                updateTimeDisplay(&systemTime);
            } else {
                updateDisplay(zones, currentZone, selectedParam);
            }
        }
        else if(buttons) {
            if(systemTime.isSettingTime) {
                if(buttons & (1<<PC5)) adjustTime(&systemTime, 5);  // V+
                if(buttons & (1<<PC6)) adjustTime(&systemTime, -5); // V-
                displayNeedsUpdate = 1;
            }
            else {
                if(buttons & (1<<PC3)) {
                    currentZone = (currentZone + 1) % NUM_ZONES;
                    // sprintf(buf, "Zone change: %d->%d", currentZone, (currentZone + 1) % NUM_ZONES);
                    // uartSendString(buf);
                    displayNeedsUpdate = 1;
                }
                if(buttons & (1<<PC4)) {
                    selectedParam = (selectedParam + 1) % 4;
                    displayNeedsUpdate = 1;
                }
                if(buttons & (1<<PC5) || buttons & (1<<PC6)) {
                    int8_t change = (buttons & (1<<PC5)) ? 1 : -1;
                    adjustParameter(&zones[currentZone], selectedParam, change);
                    displayNeedsUpdate = 1;
                }
                if(buttons & (1<<PC7)) {
                    toggleManual(&zones[currentZone], currentZone);
                    displayNeedsUpdate = 1;
                    // Отправляем статус по UART
                    sendZoneData(currentZone);
                }
            }
        }
    }
}

int main(void) {
    // Инициализация
    initPorts();
    initZones(zones);
    initSystemTime(&systemTime);
    initLcd();
    initUart();
    initDht();
    
    // Настройка таймера
    OCR0 = 57;
    TCCR0 = (1<<WGM01)|(1<<CS01)|(1<<CS00);
    TIMSK |= (1<<OCIE0);
    
    sei();
    
    // Начальное отображение
    updateDisplay(zones, currentZone, selectedParam);
    // Отправка сообщения о старте системы
    uartSendString("System started");
    
    while(1) {
        handleButtons();
        processDhtData(&zones[0], 0);
        processDhtData(&zones[1], 1);
        checkLeaks(zones);

        if(commandReady) {
            handleCommand();     // Обрабатываем команду в основном цикле
            commandReady = 0;    // Сбрасываем флаг
        }
        
        if(displayNeedsUpdate) {
            if(systemTime.isSettingTime) {
                updateTimeDisplay(&systemTime);
            } else {
                updateDisplay(zones, currentZone, selectedParam);
            }
            displayNeedsUpdate = 0;
        }
    }
    
    return 0;
}