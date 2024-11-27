#include "lcd.h"
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>

#define LCD_DATA_PORT PORTA
#define LCD_DATA_DDR  DDRA
#define LCD_CTRL_PORT PORTC
#define LCD_CTRL_DDR  DDRC

#define LCD_RS 0
#define LCD_RW 1
#define LCD_EN 2

static void lcd_cmd(uint8_t cmd) {
    LCD_CTRL_PORT &= ~((1<<LCD_RS)|(1<<LCD_RW));
    LCD_DATA_PORT = cmd;
    LCD_CTRL_PORT |= (1<<LCD_EN);
    _delay_us(1);
    LCD_CTRL_PORT &= ~(1<<LCD_EN);
    _delay_ms(2);
}

static void lcd_data(uint8_t data) {
    LCD_CTRL_PORT |= (1<<LCD_RS);
    LCD_CTRL_PORT &= ~(1<<LCD_RW);
    LCD_DATA_PORT = data;
    LCD_CTRL_PORT |= (1<<LCD_EN);
    _delay_us(1);
    LCD_CTRL_PORT &= ~(1<<LCD_EN);
    _delay_us(50);
}

void initLcd(void) {
    LCD_DATA_DDR = 0xFF;
    LCD_CTRL_DDR |= (1<<LCD_RS)|(1<<LCD_RW)|(1<<LCD_EN);
    
    _delay_ms(20);
    lcd_cmd(0x38);
    _delay_ms(5);
    lcd_cmd(0x0C);
    lcd_cmd(0x06);
    lcd_cmd(0x01);
    _delay_ms(2);
}

void lcd_string(const char *str) {
    while(*str) lcd_data(*str++);
}

void lcd_goto(uint8_t row, uint8_t col) {
    lcd_cmd(0x80 | (row * 0x40 + col));
}

void updateTimeDisplay(SystemTime* time) {
    char buf[16];
    lcd_cmd(0x01);  // Clear display
    
    if(time->isSettingTime) {
        lcd_string("Set Time:");
    } else {
        lcd_string("Time:");
    }
    
    sprintf(buf, "%02d:%02d:%02d", time->hours, time->minutes, time->seconds);
    lcd_goto(0, 6);
    lcd_string(buf);
}

void updateDisplay(Zone* zones, uint8_t currentZone, uint8_t selectedParam) {
    char buf[16];
    lcd_cmd(0x01);
    
    sprintf(buf, "Zone%d %02d:%02d/%d%%", 
            currentZone + 1,
            zones[currentZone].startHour,
            zones[currentZone].startMinute,
            zones[currentZone].humidity);
    lcd_string(buf);
    
    lcd_goto(1, 0);
    switch(selectedParam) {
        case PARAM_SCHEDULE:
            sprintf(buf, "Start: %02d:%02d", 
                    zones[currentZone].startHour,
                    zones[currentZone].startMinute);
            break;
            
        case PARAM_HUMIDITY:
            sprintf(buf, "Target Hum: %d%%", 
                    zones[currentZone].targetHumidity);
            break;
            
        case PARAM_TIME:
            sprintf(buf, "Duration: %dm", 
                    zones[currentZone].wateringTime);
            break;
            
        case PARAM_FLOW:
            sprintf(buf, "Flow: %d L/min", 
                    zones[currentZone].flowRate);
            break;
    }
    lcd_string(buf);
}

void displayError(const char* message) {
    lcd_cmd(0x01);
    lcd_string("Error:");
    lcd_goto(1, 0);
    lcd_string(message);
}