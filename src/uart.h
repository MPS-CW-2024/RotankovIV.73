#pragma once
#include "zone.h"

void initUart(void);
void uart_tx(char data);
void uart_string(const char* str);
void sendZoneStatus(Zone* zone, uint8_t zoneNum);