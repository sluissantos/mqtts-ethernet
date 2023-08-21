#ifndef DEVICE_H
#define DEVICE_H
/**
 *  Propriétario: LogPyx
 *  
 *  Nome: device.h
 *  
 *  Descrição: Contém as funções para gerenciar os dispositivos periféricos conectados.
 * 
 *  Referências:*
 *  
 *  Desenvolvedor: Henrique Ferreira 
 */

////////////////////////////////////////////////////////////////////////////////////
//Include
////////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "communication.h"
#include "bleuartServer.h"
#include "blemanager.h"

//Includes System
////////////////////////////////////////////////////////////////////////////////////
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "sdkconfig.h"

#include <time.h>
////////////////////////////////////////////////////////////////////////////////////
//Defines
///////////////////////////////////////////////////////////////////////////////////
#define DEVICE_QNT 4                // Quantidade de dispositivos 

#define DEVICE_STATUS_CONNECTING         1
#define DEVICE_STATUS_WAIT_FOR_CONECTING 2
#define DEVICE_STATUS_CONNECTED          3

#define DEVICE_TIMEOUT_LIMITE            3 // 10 * 500ms = 5s
////////////////////////////////////////////////////////////////////////////////////
//Macros
////////////////////////////////////////////////////////////////////////////////////
#define DEVICE_DEFAULT() {                                  \
    .interface =1, /*COMM_INTERFACE_BLE*/                   \
    .deviceName="",                                         \
    .gattsConnectionID = -1,                                \
    .isConnected = 0,                                       \
    .channel = 0,                                           \
    .status =DEVICE_STATUS_WAIT_FOR_CONECTING,              \
    .timeout = 0,                                           \
    .dataReady = 0,                                         \
    .connID = 0xFFFF,                                       \
}

////////////////////////////////////////////////////////////////////////////////////
//Struct
////////////////////////////////////////////////////////////////////////////////////
typedef struct{
    uint8_t interface;
    char deviceName[20];
    int16_t gattsConnectionID;
    uint8_t channel;
    uint8_t isConnected;
    uint8_t status;
    uint8_t timeout;
    uint8_t dataReady;
    uint16_t connID;

}device_t;

////////////////////////////////////////////////////////////////////////////////////
//Functions
////////////////////////////////////////////////////////////////////////////////////
void deviceInit();
void deviceConnectionTask(void * pvParameter);
int8_t deviceConnect(uint8_t newDeviceIndx);
int8_t deviceDisconnect(uint8_t deviceIndx);
device_t *deviceGet(uint8_t deviceIndx);
device_t * deviceGetCelular();
void deviceSetIndxCelular(int8_t index);
uint8_t deviceGetIndxCelular();
void deviceRotinaLed();

#endif