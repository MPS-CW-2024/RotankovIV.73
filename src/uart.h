#pragma once
#include "zone.h"
#include "time.h"
#include <util/delay.h>

// Базовые функции UART
void initUart(void);
void uartSendChar(char data);
void uartSendString(const char* str);
void sendCurrentData(void);
void sendZoneData(const uint8_t index);

// Поддерживаемые команды:
//
// Получение данных:
// data;                  - запрос состояния всех зон
//                         Ответ: Zone1: H=XX% F=YL/min A=Z
//
// Установка времени:
// time:HH:MM:SS;        - установить системное время
//                         HH (00-23), MM (00-59), SS (00-59)
//
// Управление зонами (N = 1 или 2):
// zoneN:on;            - включить помпу зоны N
// zoneN:off;           - выключить помпу зоны N
// zoneN:flow:XX;       - установить расход XX L/min (1-20)
// zoneN:start:HH:MM;   - установить время запуска
//                        HH (00-23), MM (00-59)
//
// Возможные ответы:
// OK     - команда выполнена успешно
// ON     - помпа включена
// OFF    - помпа выключена
// ERROR  - ошибка в команде
//
// Примеры:
// > data;
// < Zone1: H=45% F=2L/min A=1
// < Zone2: H=60% F=2L/min A=0
//
// > time:12:30:00;
// < OK
//
// > zone1:on;
// < ON
//
// > zone2:flow:5;
// < OK