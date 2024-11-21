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
    if (strcmp(message, "data;") == 0) {
        sendCurrentData();
        return;
    }

    // Установка времени
    int tmp_H, tmp_M, tmp_S;
    if (sscanf(message, "time:%d:%d:%d;", &tmp_H, &tmp_M, &tmp_S) == 3) {
        if (tmp_H >= 0 && tmp_H < 24 && tmp_M >= 0 && tmp_M < 60 && tmp_S >= 0 && tmp_S < 60) {
            setTime(&systemTime, tmp_H, tmp_M, tmp_S);
            uartSendString("OK");
            return;
        }
    }

    // Команды для зон
    if (strncmp(message, "zone", 4) == 0 && message[4] >= '1' && message[4] <= '2') {
        uint8_t zone = message[4] - '1';

        // Включение/выключение помпы
        if (strcmp(message + 5, ":on;") == 0) {
            zones[zone].isManual = 1;
            zones[zone].isActive = 1;
            PORTB |= (1 << zone);
            uartSendString("ON");
            return;
        }
        if (strcmp(message + 5, ":off;") == 0) {
            zones[zone].isManual = 0;
            zones[zone].isActive = 0;
            PORTB &= ~(1 << zone);
            uartSendString("OFF");
            return;
        }

        // Установка расхода
        int flow;
        if (sscanf(message + 5, ":flow:%d;", &flow) == 1) {
            if (flow >= 1 && flow <= 20) {
                zones[zone].flowRate = flow;
                uartSendString("OK");
                return;
            }
        }

        // Установка времени запуска
        int startH, startM;
        if (sscanf(message + 5, ":start:%d:%d;", &startH, &startM) == 2) {
            if (startH >= 0 && startH < 24 && startM >= 0 && startM < 60) {
                zones[zone].startHour = startH;
                zones[zone].startMinute = startM;
                uartSendString("OK");
                return;
            }
        }
    }

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