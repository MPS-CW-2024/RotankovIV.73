#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>

#define BAUD_RATE 38400

static char message[30];
static int messageIdx = 0;

extern Zone zones[NUM_ZONES];
extern SystemTime systemTime;

void uartSendChar(char data) {
    while (!(UCSRA & (1 << UDRE)));
    UDR = data;
}

void uartSendString(char* data) {
    while (*data != 0) {
        uartSendChar(*data++);
    }
    uartSendChar('\r');
    uartSendChar('\n');
    _delay_us(500);
}

void sendZoneData(const uint8_t index) {
    char buf[32];
    sprintf(buf, "Zone%d: H=%d%% F=%dL/min A=%d", 
            index + 1,
            zones[index].humidity,
            zones[index].flowRate,
            zones[index].isActive);
    uartSendString(buf);
}

void sendCurrentData(void) {
    for (uint8_t i = 0; i < NUM_ZONES; i++) {
        sendZoneData(i);
    }
}

void handleUartMessage(void) {
    // Команда запроса данных
   if(strcmp(message, "data;") == 0) {
        // При запросе данных разрешаем обновление
        zoneUpdateDisabled = 0;
        sendCurrentData();
        return;
    }

    // Обработка команд для зон
    int zone;
    char action[16]; 
    if(sscanf(message, "zone%1d:%[^;];", &zone, action) == 2 && 
       zone >= 1 && zone <= NUM_ZONES) {
       
        zone--;
        
        // Отключаем обновление по таймеру
        cli();
        zoneUpdateDisabled = 1;
        
        if(strcmp(action, "on") == 0) {
            zones[zone].isManual = 1;
            zones[zone].isActive = 1;
            PORTB |= (1 << zone);
            sei();
            uartSendString("OK");
            return;
        }
        
        if(strcmp(action, "off") == 0) {
            zones[zone].isManual = 0; 
            zones[zone].isActive = 0;
            zones[zone].timeRemaining = 0;
            PORTB &= ~(1 << zone);
            sei();
            uartSendString("OK");
            return;
        }

        // После команды - оставляем обновления отключенными
        sei();
        uartSendString("OK");
        return;
    }

    // При ошибке включаем обновления
    zoneUpdateDisabled = 0;
    uartSendString("ERROR");
}

ISR(USART_RX_vect) {
    char received = UDR;

    if (received == '\n') {
        message[messageIdx] = '\0';
        handleUartMessage();
        messageIdx = 0;
    }
    else if ((received != '\r') && (messageIdx < sizeof(message) - 1)) {
        message[messageIdx++] = received;
    }
}

void initUart(void) {
    uint16_t baudRateForUbbr = (F_CPU / (BAUD_RATE * 16UL)) - 1;
    
    UBRRH = (unsigned char)(baudRateForUbbr >> 8);
    UBRRL = (unsigned char)baudRateForUbbr;

    UCSRB = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
}