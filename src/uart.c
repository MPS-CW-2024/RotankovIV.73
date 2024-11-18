#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#define BAUD 38400
#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)

static char rxBuffer[32];
static uint8_t rxIndex = 0;

void initUart(void) {
    UBRRL = (uint8_t)UBRR_VALUE;
    UBRRH = (uint8_t)(UBRR_VALUE >> 8);
    
    UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE);
    UCSRC = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);
}

void uart_tx(char data) {
    while(!(UCSRA & (1<<UDRE)));
    UDR = data;
}

void uart_string(const char* str) {
    while(*str) uart_tx(*str++);
}

ISR(USART_RX_vect) {
    char data = UDR;
    
    if(data == '\n' || data == '\r') {
        if(rxIndex > 0) {
            rxBuffer[rxIndex] = 0;
            rxIndex = 0;
        }
    } else if(rxIndex < sizeof(rxBuffer)-1) {
        rxBuffer[rxIndex++] = data;
    }
}

void sendZoneStatus(Zone* zone, uint8_t zoneNum) {
    char buf[32];
    sprintf(buf, "Z%d,%d,%d,%d,%d\r\n",
            zoneNum + 1,
            zone->humidity,
            zone->targetHumidity,
            zone->isActive,
            zone->flowCount);
    uart_string(buf);
}