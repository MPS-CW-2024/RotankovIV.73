#pragma once
#include "zone.h"

void initLcd(void);
void lcd_string(const char *str);
void lcd_goto(uint8_t row, uint8_t col);

// Функция отображения
void updateDisplay(Zone* zones, uint8_t currentZone, uint8_t selectedParam);