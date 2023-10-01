#ifndef COMMUNICATION_H
#define COMMUNICATION_H
/**
 *  Propriétario: LogPyx
 *  
 *  Nome: communication.h
 *  
 *  Descrição: Contém as declarações das funções para tratar comunicação dos dispositivos.
 *  Este módulo é responsável por, estabelecer um protocolo de comunicação entre dispositvos,
 *  realizar leitura de buffer de dados provinientes das interfaces disponíveis.
 * 
 *  Referências:*
 *  
 *  Desenvolvedor: Henrique Ferreira, 2022
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "cJSON.h"
#include "eth.h"
#include "nvs.h"
#include "nvs_flash.h"

#define COMM_INTERFACE_UART 0
#define COMM_INTERFACE_BLE_SERVER  1  

#define DUMMY_RESPONSE 1

void commUpdateBufferTask(void * pvParameter);
void commUpdateRedeTask(void *pvParameter);
int8_t commBufferPush(uint8_t  interface,uint8_t * newData);
void commSendDataInterface(uint8_t *Dado, uint8_t length);
void set_variables(char *payload);
void commSendHexDataWithDelay(void);
void initialize_comunication();
void set_message_ip(bool flag);
void status_ip(bool flag_ip);
void store_communication_id(uint8_t id);
void commPublishStatusTask(void *pvParameter);
void set_message_offline();
uint8_t return_id();

#endif