#ifndef UART_H
#define UART_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/uart.h"
#define UART0_INSTANCE 0 // verificar circuito
#define UART0_PIN_RX   1 // verificar circuito
#define UART0_PIN_TX   2 // verificar circuito
#define UART0_BAUD_RATE 9600
#define UART2_INSTANCE 2 // verificar circuito
#define UART2_PIN_RX  16//19
#define UART2_PIN_TX  17
#define UART2_BAUD_RATE 9600
#define UART_BUFFER_SIZE 1024
#define UART_CONFIG_DEFAULT() {            \
    .baud_rate = UART2_BAUD_RATE,          \
    .data_bits = UART_DATA_8_BITS,         \
    .parity    = UART_PARITY_DISABLE,      \
    .stop_bits = UART_STOP_BITS_1,         \
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, \
    .source_clk = UART_SCLK_APB,           \
}

void uartInit(uint8_t uart);
uint8_t uartSendData(uint8_t uartInstace, char *data, uint8_t length);
uint8_t uartReadData(uint8_t uartInstace, uint8_t *data);

#endif