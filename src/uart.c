#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>

// Определение скорости передачи
#define BAUD_RATE 38400

// Буфер для приема сообщений
static char message[30];
static int messageIdx = 0;

// Глобальные переменные из main
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

// Отправка данных о зоне
void sendZoneData(const uint8_t index) {
    char buf[32];
    sprintf(buf, "Zone%d: H=%d%% F=%dL/min A=%d", 
            index + 1,
            zones[index].humidity,
            zones[index].flowRate,
            zones[index].isActive);
    uartSendString(buf);
}

// Отправка данных о всех зонах
void sendCurrentData(void) {
    for (uint8_t i = 0; i < NUM_ZONES; i++) {
        sendZoneData(i);
    }
}

// Обработка полученного сообщения
void handleUartMessage(void) {
    int tmp_H = 0;
    int tmp_M = 0;
    int tmp_S = 0;

    // Команда запроса данных
    if (strcmp(message, "data;") == 0) {
        sendCurrentData();
        return;
    }

    // Установка времени
    if (sscanf(message, "time:%d:%d:%d;", &tmp_H, &tmp_M, &tmp_S) == 3) {
        if (!((0 <= tmp_H && tmp_H < 24) && (0 <= tmp_M && tmp_M < 60) &&
              (0 <= tmp_S && tmp_S < 60))) {
            uartSendString("ERROR");
            return;
        }
        setTime(&systemTime, tmp_H, tmp_M, tmp_S);
        uartSendString("OK");
        return;
    }

    // Команды управления зонами
    if (strncmp(message, "zone", 4) == 0) {
        unsigned int tmp_zone;
        if (sscanf(message, "zone%u:", &tmp_zone) == 1 && tmp_zone > 0 && tmp_zone <= NUM_ZONES) {
            uint8_t zone = (uint8_t)(tmp_zone - 1);

            // Включение помпы
            if (strstr(message, "pump:on;")) {
                zones[zone].isManual = 1;
                zones[zone].isActive = 1;
                toggleManual(&zones[zone], zone);
                uartSendString("OK");
                return;
            }

            // Выключение помпы
            if (strstr(message, "pump:off;")) {
                zones[zone].isManual = 0;
                zones[zone].isActive = 0;
                toggleManual(&zones[zone], zone);
                uartSendString("OK");
                return;
            }

            // Установка расхода
            uint8_t flow;
            if (sscanf(message, "zone%*d:flow:%hhu;", &flow) == 1) {
                if (flow >= 1 && flow <= 20) {
                    zones[zone].flowRate = flow;
                    uartSendString("OK");
                    return;
                }
            }

            // Установка времени старта
            uint8_t startH, startM;
            if (sscanf(message, "zone%*d:start:%hhu:%hhu;", &startH, &startM) == 2) {
                if (startH < 24 && startM < 60) {
                    zones[zone].startHour = startH;
                    zones[zone].startMinute = startM;
                    uartSendString("OK");
                    return;
                }
            }
        }
    }

    uartSendString("ERROR");
}

// Обработчик прерывания приема
ISR(USART_RX_vect) {
    char received = UDR;

    if (received == '\n') {
        message[messageIdx] = '\0';
        handleUartMessage();
        messageIdx = 0;
        message[messageIdx] = '\0';
    }
    else if ((received != '\r') && (messageIdx < sizeof(message) - 1)) {
        message[messageIdx++] = received;
    }
}

// Инициализация UART
void initUart(void) {
    uint16_t baudRateForUbbr = (F_CPU / (BAUD_RATE * 16UL)) - 1;
    
    UBRRH = (unsigned char)(baudRateForUbbr >> 8);
    UBRRL = (unsigned char)baudRateForUbbr;

    UCSRB = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
}