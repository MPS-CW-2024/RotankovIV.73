#pragma once
#include "zone.h"
#include "time.h"
#include <util/delay.h>

// Функции UART
void initUart(void);
void uartSendChar(char data);
void uartSendString(char* str);
void sendCurrentData(void);
void sendZoneData(const uint8_t index);

// Поддерживаемые команды:
// data;                  - получить данные о всех зонах
// time:HH:MM:SS;        - установить время
// zoneN:pump:on;        - включить помпу зоны N
// zoneN:pump:off;       - выключить помпу зоны N
// zoneN:flow:XX;        - установить расход XX L/min для зоны N
// zoneN:start:HH:MM;    - установить время запуска для зоны N

// Формат ответов:
// OK     - команда выполнена успешно
// ERROR  - ошибка в команде