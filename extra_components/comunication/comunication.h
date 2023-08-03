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

#define COMM_INTERFACE_UART 0
#define COMM_INTERFACE_BLE_SERVER  1  
#define COMM_INTERFACE_BLE_CLIENT_1 2  
#define COMM_INTERFACE_BLE_CLIENT_2 3
#define COMM_INTERFACE_BLE_CLIENT_3 4
   
// OPCODE - RD sinaliza devolução/leitura de dados
//          WR sinaliza escrita de dados
#define RD_REQUEST_CONNECTION    0xD0
#define RD_ALL_INFO_DEVICE       0xD1
#define WR_TAG_ID_DISTANCE       0xD2
#define WR_ZONE_FAR_DISTANCE     0xD3
#define WR_ZONE_NEAR_DISTANCE    0XD4
#define WR_PERIOD_REQUEST        0xD5   
#define WR_PERIPHERAL_OFFSET     0xD6
#define WR_DECA_ID_LIST          0xD7
#define RD_REQUEST_DISCONNECTION 0xD8
#define WR_ONOFF_CAM             0XA1
#define RD_TEMP_CAM              0XA2
#define WR_OFFSET_MAG_ANGLE      0xA6
#define WR_FORK_PARAM            0xA7
#define WR_PICK_PERIOD           0xA8
#define WR_START_SCAN            0xA9
#define WR_VERBOSE               0xAA
#define WR_CONTROL_APPID         0xDB
#define WR_CHRANGE_APPID         0xDC
#define WR_DISTANCE              0xD1
#define WR_CH0_BARCODE           0xD2
#define WR_CH1_BARCODE           0xD3
#define WR_RECEIVE_LOAD_DATA     0xB0
#define WR_OPERATION_ERRO         0xE0
#define ERRO_DEVICE_NOT_CONECTED  0x01
#define ERRO_DEVICE_NOT_ECHO      0x02
#define STX 0x02
#define RTX 0x03
// Distance Alarm
#define ALARM_RLA_PIN       16
#define ALARM_RLB_PIN       4
#define DUMMY_RESPONSE    0

void commUpdateBufferTask(void * pvParameter);
int8_t commBufferPush(uint8_t  interface,uint8_t * newData);
void commSendDataInterface(uint8_t *Dado, uint8_t length);
void set_variables(char *payload);
void commSendHexDataWithDelay(void);
void initialize_comunication(uint8_t id_d);

#endif