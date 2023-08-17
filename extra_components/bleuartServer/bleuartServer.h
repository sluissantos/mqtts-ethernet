#ifndef BLEUARTSERVER_H
#define BLEUARTSERVER_H
/**
 *  Propriétario: LogPyx
 *  
 *  Nome: app_ble_uart_c.h
 *  
 *  Descrição: Contém as funções necessárias para iniciar/operar o serviço de uart via BLE.
 *  Esta implementação adota como base Nordic Uart Service, e atua tanto com servidor como cliente (periférico/central)
 * 
 *  Referências:
 *  
 *  Desenvovedor: Henrique Ferreira 
 */

////////////////////////////////////////////////////////////////////////////////////
//Include
////////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_gap_ble_api.h"
////////////////////////////////////////////////////////////////////////////////////
//Defines
////////////////////////////////////////////////////////////////////////////////////
#define ADV_DATA_MIN_INTERVAL           0x10             /*slave connection min interval, Time = min_interval * 1.25 msec*/
#define ADV_DATA_MAX_INTERVAL           0x3C             /*slave connection max interval, Time = max_interval * 1.25 msec*/
#define ADV_DATA_PARAM_MIN_INTERVAL     0x20             /*Connection param min interval, Time = min_interval * 0.625 msec*/
#define ADV_DATA_PARAM_MAX_INTERVAL     0x78             /*Connection param min interval, Time = max_interval * 0.625 msec*/
#define GATTS_NUM_CONN_MAX                 4             /*Número máximo de conexões permitido simultaneas no server*/
#define ADV_DATA_APPERANCE              0x00
#define ADV_DATA_UUID_LEN               ESP_UUID_LEN_128 /*Tamanho do uuid usado no serviço*/
////////////////////////////////////////////////////////////////////////////////////
//Macros
////////////////////////////////////////////////////////////////////////////////////

#define GATT_ADV_DATA_DEFAULT(UUID) {                                       \
    .set_scan_rsp = 0,                                                      \
    .include_name = 0,                                                      \
    .include_txpower = 0,                                                   \
    .min_interval = ADV_DATA_MIN_INTERVAL,                                  \
    .max_interval = ADV_DATA_MAX_INTERVAL,                                  \
    .appearance = ADV_DATA_APPERANCE,                                       \
    .manufacturer_len = 0,                                                  \
    .p_manufacturer_data =  NULL,                                           \
    .service_data_len = 0,                                                  \
    .p_service_data = NULL,                                                 \
    .service_uuid_len = 16,                                                 \
    .p_service_uuid = (uint8_t *)UUID,                                      \
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),   \
}

#define GATT_ADV_DATA_RESP_DEFAULT(UUID) {                                  \
    .set_scan_rsp = 1,                                                      \
    .include_name = 1,                                                      \
    .include_txpower = 1,                                                   \
    .appearance = ADV_DATA_APPERANCE,                                       \
    .manufacturer_len = 0,                                                  \
    .p_manufacturer_data =  NULL,                                           \
    .service_data_len = 0,                                                  \
    .p_service_data = NULL,                                                 \
    .service_uuid_len = ADV_DATA_UUID_LEN,                                  \
    .p_service_uuid = (uint8_t *)UUID,                                      \
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),   \
}
/*Configuração padrão para os pacotes advertising*/
#define GATT_ADV_PARAMS_DEFAULT() {                         \
    .adv_int_min = ADV_DATA_PARAM_MIN_INTERVAL,             \
    .adv_int_max = ADV_DATA_PARAM_MAX_INTERVAL,             \
    .adv_type = ADV_TYPE_IND,                               \
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,                  \
    .channel_map = ADV_CHNL_ALL,                            \
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY, \
}
////////////////////////////////////////////////////////////////////////////////////
//Functions - 
////////////////////////////////////////////////////////////////////////////////////

void bleuartServerInit(uint16_t ID);

void gatt_uart_send_data(uint8_t * data, uint16_t length, uint8_t connID);

void gatts_setReceiveDataCallback(int8_t (*externReceiverCallback)(uint8_t *data));

int8_t gatt_uart_getConnectionsAvailables();

int8_t gattuartUnregisterDevice(uint16_t connID);

int8_t gattuartRegisterDevice(uint8_t deviceIndx);

// Get e Set
uint8_t bleuartServerGetScanRspConfig();

uint8_t bleuartServerGetAdvConfigDone();

void bleuartServerSetAdvConfigDone(uint8_t newStatus);

esp_ble_adv_params_t * bleuartServerGetAdvParam();

#endif