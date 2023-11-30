/**
 *  Propriétario: LogPyx
 *  Nome: uart.c
 *  Descrição: Trata funções relacionadas aos devices (periféricos), gerenciamento de conexão,
 *  configuração, infomarções sobre.  
 *  Referências:
 *  Desenvolvedor: Henrique Ferreira 
 */

#include "uart.h"

uart_config_t uart_config = UART_CONFIG_DEFAULT();
int intr_alloc_flags =0;

///////////////////////////////////////////////////////////////////////////////////
// Função: uart_Init(uint8_t uart);
// Descrição: Verifica se o device conectado possui um decawaveID válido.
// Argumentos: uint8_t uart -> UART0_INSTANCE
//                             UART1_INSTANCE
// Retorno:    void 
//////////////////////////////////////////////////////////////////////////////////
 void uartInit(uint8_t uart){
    switch (uart){
    case UART0_INSTANCE:
        ESP_ERROR_CHECK(uart_driver_install(UART0_INSTANCE, UART_BUFFER_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
        ESP_ERROR_CHECK(uart_param_config(UART0_INSTANCE, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(UART0_INSTANCE, UART0_PIN_TX,UART0_PIN_RX, -1, -1));
        break;

    case UART2_INSTANCE:
        ESP_ERROR_CHECK(uart_driver_install(UART2_INSTANCE, UART_BUFFER_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
        ESP_ERROR_CHECK(uart_param_config(UART2_INSTANCE, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(UART2_INSTANCE, UART2_PIN_TX,UART2_PIN_RX, -1, -1));
        break;
    
    default:
        break;
    }
 }

///////////////////////////////////////////////////////////////////////////////////
// Função: uartSendData(uint8_t uartInstace, uint8_t *data, uint8_t lenght);
// Descrição: Verifica se o device conectado possui um decawaveID válido.
// Argumentos: uint8_t uart -> UART0_INSTANCE
//                             UART1_INSTANCE
// Retorno:    void 
//////////////////////////////////////////////////////////////////////////////////
uint8_t uartSendData(uint8_t uartInstace, char *data, uint8_t length) {
    // Write data back to the UART
    return uart_write_bytes(uartInstace, data, length);
}

///////////////////////////////////////////////////////////////////////////////////
// Função: uartReadData(uint8_t uartInstace, uint8_t *data, uint8_t tam);
// Descrição: Verifica se o device conectado possui um decawaveID válido.
// Argumentos: uint8_t uart   -> UART0_INSTANCE
//                               UART1_INSTANCE
//             uint8_t * data -> buffer para recepeção de dados 
// Retorno:    void 
//////////////////////////////////////////////////////////////////////////////////
uint8_t uartReadData(uint8_t uartInstace, uint8_t *data){
    int len= uart_read_bytes(uartInstace, data, (UART_BUFFER_SIZE - 1), 20 / portTICK_PERIOD_MS);
    return len;
}