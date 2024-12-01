#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>

#define BAUD_RATE 38400

static char message[30];
static int messageIdx = 0;
volatile uint8_t commandReady = 0;

extern Zone zones[NUM_ZONES];
extern SystemTime systemTime;

// Простая отправка символа
void uartSendChar(char data) {
    while (!(UCSRA & (1 << UDRE)));
    UDR = data;
}

// Простая отправка строки
void uartSendString(const char* str) {
    while(*str) {
        uartSendChar(*str++);
    }
    uartSendChar('\r');
    uartSendChar('\n');
}

// Отправка числа напрямую без sprintf
static void uartSendNumber(uint8_t num) {
    // Для чисел до 255
    char buf[4];  // максимум 3 цифры + \0
    uint8_t i = 0;
    
    // Преобразуем число в строку
    do {
        buf[i++] = '0' + (num % 10);
        num = num / 10;
    } while(num > 0);
    
    // Отправляем в обратном порядке
    while(i > 0) {
        uartSendChar(buf[--i]);
    }
}

void sendZoneData(const uint8_t index) {
    uint8_t sreg = SREG;
    cli();
    
    // Сначала копируем все данные локально
    uint8_t humidity = zones[index].humidity;
    uint8_t flowRate = zones[index].flowRate;
    uint8_t isActive = zones[index].isActive;
    
    SREG = sreg;
    
    // Отправляем данные напрямую, без формирования строки
    uartSendChar('Z');
    uartSendChar('o');
    uartSendChar('n');
    uartSendChar('e');
    uartSendNumber(index + 1);
    uartSendChar(':');
    uartSendChar(' ');
    uartSendChar('H');
    uartSendChar('=');
    uartSendNumber(humidity);
    uartSendChar("%");
    uartSendChar(' ');
    uartSendChar('F');
    uartSendChar('=');
    uartSendNumber(flowRate);
    uartSendChar(' ');
    uartSendChar('L');
    uartSendChar('/');
    uartSendChar("m");
    uartSendChar('i');
    uartSendChar('n');
    uartSendChar(' ');
    uartSendChar('A');
    uartSendChar('=');
    uartSendNumber(isActive);
    uartSendString("");
}

void handleCommand(void) {
    // Команда запроса данных
    if(strcmp(message, "data;") == 0) {
        uartSendString("---Data---");
        // Отправляем данные по каждой зоне
        for(uint8_t i = 0; i < NUM_ZONES; i++) {
            sendZoneData(i);
        }
        uartSendString("---End---");
        return;
    }

    // Обработка команды установки времени
    int hour, min, sec;
    if(sscanf(message, "time:%d:%d:%d;", &hour, &min, &sec) == 3) {
        if(hour >= 0 && hour < 24 && min >= 0 && min < 60 && sec >= 0 && sec < 60) {
            uint8_t sreg = SREG;
            cli();
            setTime(&systemTime, hour, min, sec);
            SREG = sreg;
            uartSendString("OK");
            return;
        }
    }

    // Обработка команд для зон
    int zone;
    char cmd[10];
    if(sscanf(message, "zone%d:%[^;];", &zone, cmd) == 2 && 
       zone >= 1 && zone <= NUM_ZONES) {
        
        zone--; // Преобразуем в индекс массива
        uint8_t sreg = SREG;
        
        if(strcmp(cmd, "on") == 0) {
            cli();
            zones[zone].isManual = 1;
            zones[zone].isActive = 1;
            PORTB |= (1 << zone);
            SREG = sreg;
            uartSendString("ON");
            return;
        }
        else if(strcmp(cmd, "off") == 0) {
            cli();
            zones[zone].isManual = 0;
            zones[zone].isActive = 0;
            zones[zone].timeRemaining = 0;
            PORTB &= ~(1 << zone);
            SREG = sreg;
            uartSendString("OFF");
            return;
        }else if(strncmp(cmd, "flow:", 5) == 0) {
            // Parse flow rate
            int flowRate;
            if(sscanf(cmd + 5, "%d", &flowRate) == 1 && flowRate >=1 && flowRate <= 20) {
                uint8_t sreg = SREG;
                cli();
                zones[zone].flowRate = flowRate;
                SREG = sreg;
                uartSendString("OK");
                return;
            }
        }
        else if(strncmp(cmd, "start:", 6) == 0) {
            // Parse start time
            int startHour, startMin;
            if(sscanf(cmd + 6, "%d:%d", &startHour, &startMin) == 2 &&
               startHour >= 0 && startHour < 24 && startMin >= 0 && startMin < 60) {
                uint8_t sreg = SREG;
                cli();
                zones[zone].startHour = startHour;
                zones[zone].startMinute = startMin;
                SREG = sreg;
                uartSendString("OK");
                return;
            }
        }
    }

    uartSendString("ERROR");
}

ISR(USART_RX_vect) {
    char received = UDR;

    if(received == '\n') {
        message[messageIdx] = '\0';
        commandReady = 1;  // Устанавливаем флаг готовности команды
        messageIdx = 0;
    }
    else if(received != '\r' && messageIdx < sizeof(message) - 1) {
        message[messageIdx++] = received;
    }
}

void initUart(void) {
    uint16_t baudRateForUbbr = (F_CPU / (BAUD_RATE * 16UL)) - 1;
    
    UBRRH = (unsigned char)(baudRateForUbbr >> 8);
    UBRRL = (unsigned char)baudRateForUbbr;

    UCSRB = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
    
    uartSendString("UART OK");
}