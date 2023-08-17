/**
 *  Propriétario: LogPyx
 *  
 *  Nome: app_ble_uart_c.c
 *  
 *  Descrição: Contém as implementações das funções para serviço de uart via BLE em como Central.
 *  Esta implementação adota como base Nordic Uart Service
 * 
 *  Referências:
 *
 *  Desenvovedor: Henrique Ferreira 
 */

////////////////////////////////////////////////////////////////////////////////////
//Includes Aura
////////////////////////////////////////////////////////////////////////////////////
#include "bleuartServer.h"
////////////////////////////////////////////////////////////////////////////////////
//Includes System
////////////////////////////////////////////////////////////////////////////////////
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_bt.h"


#include "esp_gatts_api.h"
#include "esp_gattc_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "sdkconfig.h"
#include "communication.h"
#include "device.h"

////////////////////////////////////////////////////////////////////////////////////
//Private Defines
////////////////////////////////////////////////////////////////////////////////////
#define GATTS_TAG "BleuartServer"

#define GATTS_CHAR_UUID_UART_TX     0x0003    // UUID de 16 bits
#define GATTS_CHAR_UUID_UART_RX     0x0002    // UUID de 16 bits
#define GATTS_DESCR_UUID_TEST_A     0x3333    // UUID de 16 bits

#define GATTS_NUM_HANDLE_TEST_A     6         // Qunatidade de handles necessários para montar a tabela ATT
#define GATTS_NUM_OF_CHAR           2         // Quantidade de caracteristicas contidas no serviço

#define TEST_DEVICE_NAME           "AURA"    // Nome do dispositivo
#define TEST_MANUFACTURER_DATA_LEN  17        // Quantidades de bytes para custom data - Não utlizado

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40      // Quantidade máxima de bytes que uma caraterisca pode enviar por vez

#define PREPARE_BUF_MAX_SIZE        1024

#define adv_config_flag             (1 << 0)
#define scan_rsp_config_flag        (1 << 1)

#define PROFILE_NUM                 1         // Quantidade de serviços criados no gatt server
#define PROFILE_UART_ID             0         // Identificador do profile/serviço registado no bluedroid   
#define PROFILE_B_APP_ID            1         // Identificador do segundo profile/serviço - Não utilizado

#define SLAVE_ROLE                  1

#define REMOTE_SERVICE_UUID   0x00FF
////////////////////////////////////////////////////////////////////////////////////
//Private Macros
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////
//Global Variable
////////////////////////////////////////////////////////////////////////////////////
// Define UUID usado pelo Nordic Service Uart
static uint8_t adv_service_uuid128[16] = {
        0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E,
};
// Contém os dados enviados via broadcast (advertising)
static esp_ble_adv_data_t adv_data = GATT_ADV_DATA_DEFAULT(&adv_service_uuid128[0]);
// Contém os dados enviados de resposta via broadcast (advertising)
static esp_ble_adv_data_t scan_rsp_data = GATT_ADV_DATA_RESP_DEFAULT(&adv_service_uuid128[0]);
// Contém os parâmetros de conexão dos pacotes. EX contém tempos  minimos maximos de respota de um pacote
static esp_ble_adv_params_t adv_params = GATT_ADV_PARAMS_DEFAULT();

// Carrega propriedades de escrita, leitura das caracteristicas
static esp_gatt_char_prop_t tx_property = 0;
static esp_gatt_char_prop_t rx_property = 0;

// Carrega valor default da caracteristica TX
static uint8_t char_value_default = 0xFF;
static esp_attr_value_t gatts_demo_char1_val =
{
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(char_value_default),
    .attr_value   = &char_value_default,
};

// Contador utilizado na criação das características
uint8_t gatt_char_count_succes = 0;
int8_t gatt_conn_count =-1;                        // Contabiliza o número de conexões disponiveis
// Flag utilizana no eventos do gap handler
static uint8_t adv_config_done = 0;

// Contém variáveis para manipular o profile
struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;                        // CallBack handler
    uint16_t gatts_if;                              // Gatt Interface
    uint16_t app_id;                                // Application ID
    uint16_t conn_id[GATTS_NUM_CONN_MAX];           // Connection ID
    uint16_t service_handle;                        // Service Handle 
    esp_gatt_srvc_id_t service_id;                  // Service ID
    uint16_t char_tx_handle[GATTS_NUM_OF_CHAR];     // Char handle
    esp_bt_uuid_t char_tx_uuid[GATTS_NUM_OF_CHAR];  // Char uuid
    esp_gatt_perm_t perm;                           // Attribute permissions
    esp_gatt_char_prop_t property;                  // Char properties
    uint16_t descr_handle;                          // Client Characteristic Configuration descriptor handle 
    esp_bt_uuid_t descr_uuid;                       // Client Characteristic Configuration descriptor UUID
};

// Função de callback disparada nos eventos profile uart 
static void gatts_uart_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
//static void gatts_profile_b_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

// Incializa um vetor contendo todos os profiles disponíveis no gatt server
/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_UART_ID] = {
        .gatts_cb = gatts_uart_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,                    // Not get the gatt_if, so initial is ESP_GATT_IF_NONE
    },
    /*[PROFILE_B_APP_ID] = {
        .gatts_cb = gatts_profile_b_event_handler,       // This demo does not implement, similar as profile A 
        .gatts_if = ESP_GATT_IF_NONE,                    // Not get the gatt_if, so initial is ESP_GATT_IF_NONE 
    },*/
};

uint16_t freeConection[GATTS_NUM_CONN_MAX]={0};

typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;

static prepare_type_env_t a_prepare_write_env;

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

int8_t (*gatts_receiverCallback)(uint8_t * data);

char deviceName[15];

////////////////////////////////////////////////////////////////////////////////////
//Implementations - Server Bluetooth - INICIO
////////////////////////////////////////////////////////////////////////////////////
void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    esp_gatt_status_t status = ESP_GATT_OK;
    if (param->write.need_rsp){
        if (param->write.is_prep){
            if (prepare_write_env->prepare_buf == NULL) {
                prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE*sizeof(uint8_t));
                prepare_write_env->prepare_len = 0;
                if (prepare_write_env->prepare_buf == NULL) {
                   // ESP_LOGE(GATTS_TAG, "Gatt_server prep no mem\n");
                    status = ESP_GATT_NO_RESOURCES;
                }
            } else {
                if(param->write.offset > PREPARE_BUF_MAX_SIZE) {
                    status = ESP_GATT_INVALID_OFFSET;
                } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
                    status = ESP_GATT_INVALID_ATTR_LEN;
                }
            }

            esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK){
              // ESP_LOGE(GATTS_TAG, "Send response error\n");
            }
            free(gatt_rsp);
            if (status != ESP_GATT_OK){
                return;
            }
            memcpy(prepare_write_env->prepare_buf + param->write.offset,
                   param->write.value,
                   param->write.len);
            prepare_write_env->prepare_len += param->write.len;

        }else{
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
        }
    }
}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC){
      //  esp_log_buffer_hex(GATTS_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }else{
      //  ESP_LOGI(GATTS_TAG,"ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_uart_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
   
    case ESP_GATTS_REG_EVT:{
       // ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gl_profile_tab[PROFILE_UART_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_UART_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_UART_ID].service_id.id.uuid.len = ESP_UUID_LEN_128;
      
        memcpy(&gl_profile_tab[PROFILE_UART_ID].service_id.id.uuid.uuid.uuid128[0],&adv_service_uuid128[0],16);

        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(deviceName);
        if (set_dev_name_ret){
       //     ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }

        //config adv data
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret){
      //      ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
        }
        adv_config_done |= adv_config_flag;
        //config scan response data
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret){
      //      ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
        }
        adv_config_done |= scan_rsp_config_flag;

        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_UART_ID].service_id, GATTS_NUM_HANDLE_TEST_A);
        }
        break;
    case ESP_GATTS_READ_EVT: {
  //      ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 4;
        rsp.attr_value.value[0] = 'T';
        rsp.attr_value.value[1] = 'E';
        rsp.attr_value.value[2] = 'S';
        rsp.attr_value.value[3] = 'T';
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        }
        break; 
    case ESP_GATTS_WRITE_EVT: {
       // ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep){
          // ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
          #if(DUMMY_RESPONSE)
                ESP_LOGI(GATTS_TAG, "message=%.*s", param->write.len, param->write.value);
                esp_log_buffer_hex("ReceivData", param->write.value, param->write.len);
                if ((int)param->write.value[0] == 0){
                    ESP_LOGI(GATTS_TAG, "entrou aqui mensagem total");
                    char* ip_string = (char*)malloc(16);
                    snprintf(ip_string, 16, "%d.%d.%d.%d", param->write.value[1], param->write.value[2], param->write.value[3], param->write.value[4]);
                    ESP_LOGI(GATTS_TAG, "ip_string = %s", ip_string);

                    char* gateway_string = (char*)malloc(16);
                    snprintf(gateway_string, 16, "%d.%d.%d.%d", param->write.value[5], param->write.value[6], param->write.value[7], param->write.value[8]);
                    ESP_LOGI(GATTS_TAG, "gateway_string = %s", gateway_string);

                    char* netmask_string = (char*)malloc(16);
                    snprintf(netmask_string, 16, "%d.%d.%d.%d", param->write.value[9], param->write.value[10], param->write.value[11], param->write.value[12]);
                    ESP_LOGI(GATTS_TAG, "netmask = %s", netmask_string);

                    char* dns_string = (char*)malloc(16);
                    snprintf(dns_string, 16, "%d.%d.%d.%d", param->write.value[13], param->write.value[14], param->write.value[15], param->write.value[16]);
                    ESP_LOGI(GATTS_TAG, "dns_string = %s", dns_string);
                    change_rede(ip_string, gateway_string, netmask_string, dns_string);
                }
                
                else if ((int)param->write.value[0] == 2){
                    ESP_LOGI(GATTS_TAG, "entrou aqui ip");
                    char* ip_string = (char*)malloc(16);
                    snprintf(ip_string, 16, "%d.%d.%d.%d", param->write.value[1], param->write.value[2], param->write.value[3], param->write.value[4]);
                    ESP_LOGI(GATTS_TAG, "ip_string = %s", ip_string);
                    change_ip(ip_string);
                }
                
                else if ((int)param->write.value[0] == 3){
                    char* gateway_string = (char*)malloc(16);
                    snprintf(gateway_string, 16, "%d.%d.%d.%d", param->write.value[1], param->write.value[2], param->write.value[3], param->write.value[4]);
                    ESP_LOGI(GATTS_TAG, "gateway_string = %s", gateway_string);
                    change_gateway(gateway_string);
                }

                else if ((int)param->write.value[0] == 4){
                    char* netmask_string = (char*)malloc(16);
                    snprintf(netmask_string, 16, "%d.%d.%d.%d", param->write.value[1], param->write.value[2], param->write.value[3], param->write.value[4]);
                    ESP_LOGI(GATTS_TAG, "netmask = %s", netmask_string);
                    change_netmask(netmask_string);
                }

                else if ((int)param->write.value[0] == 5){
                    char* dns_string = (char*)malloc(16);
                    snprintf(dns_string, 16, "%d.%d.%d.%d", param->write.value[1], param->write.value[2], param->write.value[3], param->write.value[4]);
                    ESP_LOGI(GATTS_TAG, "dns_string = %s", dns_string);
                    change_dns(dns_string);
                }

                else if ((int)param->write.value[0] == 6){
                    nvs_erase();
                }
    
                else{
                    ESP_LOGI(GATTS_TAG, "OPCODE DESCONHECIDO");
                }
          #endif
        

            if (gl_profile_tab[PROFILE_UART_ID].descr_handle == param->write.handle && param->write.len == 2){
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == 0x0001){
                    if (tx_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY){
                       ESP_LOGI(GATTS_TAG, "notify enable");
                       /* uint8_t notify_data[15];
                        notify_data[0] ='A';
                        for (int i = 0; i < sizeof(notify_data)-1; ++i)
                        {
                            notify_data[i+1] = notify_data[i]+1;
                        }
                        //the size of notify_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_UART_ID].char_tx_handle[0],
                                                sizeof(notify_data), notify_data, false);*/
                    }
                }else if (descr_value == 0x0002){
                    if (tx_property & ESP_GATT_CHAR_PROP_BIT_INDICATE){
                        ESP_LOGI(GATTS_TAG, "indicate enable");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++i)
                        {
                            indicate_data[i] = i%0xff;
                        }
                        //the size of indicate_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_UART_ID].char_tx_handle[0],
                                                sizeof(indicate_data), indicate_data, true);
                    }
                }
                else if (descr_value == 0x0000){
                    ESP_LOGI(GATTS_TAG, "notify/indicate disable ");
                }else{
                    ESP_LOGE(GATTS_TAG, "unknown descr value");
                    esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
                }

            }else{
                
                  for(uint16_t i=0; i<param->write.len;i++){
                    //commBufferPush(COMM_INTERFACE_BLE_SERVER,&param->write.value[i]);
                  }
            }
        }
        example_write_event_env(gatts_if, &a_prepare_write_env, param);
     
    }
        break;
    case ESP_GATTS_EXEC_WRITE_EVT:{
       // ESP_LOGI(GATTS_TAG,"ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&a_prepare_write_env, param);
        }
        break;
    case ESP_GATTS_MTU_EVT:{
       // ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
       }
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:{
        ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gl_profile_tab[PROFILE_UART_ID].service_handle = param->create.service_handle;
        gl_profile_tab[PROFILE_UART_ID].char_tx_uuid[0].len = ESP_UUID_LEN_128;
        gl_profile_tab[PROFILE_UART_ID].char_tx_uuid[1].len = ESP_UUID_LEN_128;
        
        // Cria caracteristicas - tx
        memcpy(&gl_profile_tab[PROFILE_UART_ID].char_tx_uuid[0].uuid.uuid128[0],&adv_service_uuid128[0],16);
        gl_profile_tab[PROFILE_UART_ID].char_tx_uuid[0].uuid.uuid128[12] = 0x03; 
        // Cria caracteristica - rx
        memcpy(&gl_profile_tab[PROFILE_UART_ID].char_tx_uuid[1].uuid.uuid128[0],&adv_service_uuid128[0],16);
        gl_profile_tab[PROFILE_UART_ID].char_tx_uuid[1].uuid.uuid128[12] = 0x02;

        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_UART_ID].service_handle);
        tx_property = ESP_GATT_CHAR_PROP_BIT_NOTIFY | ESP_GATT_CHAR_PROP_BIT_READ;
        rx_property = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
       
        esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_UART_ID].service_handle, &gl_profile_tab[PROFILE_UART_ID].char_tx_uuid[gatt_char_count_succes],
                                                        ESP_GATT_PERM_READ, //| ESP_GATT_PERM_WRITE,
                                                        tx_property,
                                                        &gatts_demo_char1_val, NULL);
        if (add_char_ret){
            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x",add_char_ret);
        }

        }
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT: {
        uint16_t length = 0;
        const uint8_t *prf_char;

        
        /*ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);*/
        
        gl_profile_tab[PROFILE_UART_ID].char_tx_handle[gatt_char_count_succes] = param->add_char.attr_handle;
        gl_profile_tab[PROFILE_UART_ID].descr_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_UART_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
        esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle,  &length, &prf_char);
        if (get_attr_ret == ESP_FAIL){
         //   ESP_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
        }

      //  ESP_LOGI(GATTS_TAG, "the gatts demo char length = %x\n", length);
        for(int i = 0; i < length; i++){
       //     ESP_LOGI(GATTS_TAG, "prf_char[%x] =%x\n",i,prf_char[i]);
        }
      
        if(gatt_char_count_succes == 0){
        esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_UART_ID].service_handle, &gl_profile_tab[PROFILE_UART_ID].descr_uuid,
                                                                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
        if (add_descr_ret){
        //    ESP_LOGE(GATTS_TAG, "add char descr failed, error code =%x", add_descr_ret);
        }}
        }
        break;
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:{
        gl_profile_tab[PROFILE_UART_ID].descr_handle = param->add_char_descr.attr_handle;
       /* ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);*/
        
            // Inicializa a criação da caracteristica RX
            gatt_char_count_succes++;
            if(gatt_char_count_succes == 1 ){
                esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_UART_ID].service_handle, &gl_profile_tab[PROFILE_UART_ID].char_tx_uuid[gatt_char_count_succes],
                                                                ESP_GATT_PERM_WRITE,
                                                                rx_property,
                                                                &gatts_demo_char1_val, NULL);
                    // esp_ble_gap_start_advertising(&adv_params);
                    if (add_char_ret){
                    //   ESP_LOGE(GATTS_TAG, "add char failed, error code =%x",add_char_ret);
                    }
            }
        }
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:{
       /* ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);*/
                }
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT: {
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x3C;    // max_int = 0x20*1.25ms = 75ms
        conn_params.min_int = 0x28;    // min_int = 0x10*1.25ms = 20ms // 50ms
        conn_params.timeout = 2000;     // timeout = 400*10ms = 4000ms // 20s
        /*ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                   param->connect.conn_id,
                   param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                   param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);*/

      
        
        if(param->connect.link_role == SLAVE_ROLE){
        
         ESP_LOGI(GATTS_TAG,"Celular: %d",param->connect.conn_id);
                // Verifica se já atingiu numero de conexões limtie
            if(gatt_conn_count < GATTS_NUM_CONN_MAX)
                gatt_conn_count++;    
            
           /* if(gatt_conn_count < 2){
                esp_ble_gap_start_advertising(&adv_params);
            }*/
            
            // Guarda o id de conexão         
            gl_profile_tab[PROFILE_UART_ID].conn_id[gatt_conn_count] = param->connect.conn_id;
            deviceGet(gatt_conn_count)->connID = param->connect.conn_id;
            deviceGet(gatt_conn_count)->isConnected =1;
            // Atualiza status de conexão disponivel
            freeConection[gatt_conn_count] =1;


            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);

            esp_power_level_t power = 0;
            esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL0, ESP_PWR_LVL_P9);
            vTaskDelay(50/portTICK_PERIOD_MS);
            power = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_CONN_HDL0);
            ESP_LOGI(GATTS_TAG, "Tx Connection0 Power: %d", power);
          }

        }

        break;
    
    case ESP_GATTS_DISCONNECT_EVT:{
       // ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
       if(param->connect.link_role == SLAVE_ROLE){
         esp_ble_gap_start_advertising(&adv_params);
        // Na ocorrencia de evento de desconexão decrementa numero de conexões
        if(gatt_conn_count >=0)
            gatt_conn_count--;
        
        gattuartUnregisterDevice(param->disconnect.conn_id);
        ESP_LOGI(GATTS_TAG,"DISCONNECT_EVT - Conexoes Disponiveis:%d ConnID", gatt_conn_count);
        }

       }
        break;
    case ESP_GATTS_CONF_EVT:{
        //ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK){
            esp_log_buffer_hex(GATTS_TAG, param->conf.value, param->conf.len);
        }
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        } else {
          /*  ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n",
                    param->reg.app_id,
                    param->reg.status);*/
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gatts_if == gl_profile_tab[idx].gatts_if) {
                if (gl_profile_tab[idx].gatts_cb) {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

///////////////////////////////////////////////////////////////////////////////////
// Função: gatts_uart_init();
// Descrição: Função responsável por iniciar o gatt server contendo o uart service.
// OBs: Componentes habilitados no processo
//      nvs, bt_controller, bluedroid stack.
// Argumentos: void
// Retorno:    void
//////////////////////////////////////////////////////////////////////////////////
void bleuartServerInit(uint16_t ID){
    esp_err_t ret;

    sprintf(deviceName,"TEST_%02X%02X",(uint8_t)(ID >> 8),(uint8_t)ID); 

     ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
       // ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
        return;
    }
  
    ret = esp_ble_gatts_app_register(0);
    if (ret){
      //  ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
        return;
    }

   /* esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(247);
    if (local_mtu_ret){
       // ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }*/
}

///////////////////////////////////////////////////////////////////////////////////
// Função: gatts_uart_send_data(uint8_t * data, uint9_t length);
// Descrição: Função responsável por enviar dados através da ble uart.
// Argumentos: uint8_t * data -> Ponteiro/vetor de dados a serem enviados
//             uint8_t length -> Quantidade de dados a serem enviados
//             uint8_t connID -> Identificador da conexão
// OBs: Utilize getConnectionAvailable() para obter conexões disponíveis
// Retorno:    void
//////////////////////////////////////////////////////////////////////////////////
void gatt_uart_send_data(uint8_t * data, uint16_t length,uint8_t connID){
    esp_err_t err_code = ESP_OK;
    // Se uart profile contem uma interface cliente envia dado.
    if(gl_profile_tab[PROFILE_UART_ID].gatts_if != ESP_GATT_IF_NONE)  
     err_code =esp_ble_gatts_send_indicate(gl_profile_tab[PROFILE_UART_ID].gatts_if,
                                           connID,//gl_profile_tab[PROFILE_UART_ID].conn_id[connID],
                                           gl_profile_tab[PROFILE_UART_ID].char_tx_handle[0],
                                           length, 
                                           data, 
                                           false);
    //ESP_LOGI(GATTS_TAG, "ConnID Send MSG %d",connID);
     //ESP_LOGI(GATTS_TAG, "Err_Code %d",err_code);
    // ESP_ERROR_CHECK(err_code);
}

///////////////////////////////////////////////////////////////////////////////////
// Função: getConnectionsAvailables();
// Descrição: Retorna retorna o ultimo indice de conexão disponível
// Argumentos: void 
// Retorno:    gatt_coon_count -> [-1 0 .. GATT_GATTS_NUM_CONN_MAX-1]
// OBs: -1 indica que não existem conexões disponíveis 
//////////////////////////////////////////////////////////////////////////////////
int8_t gatt_uart_getConnectionsAvailables(){
    return gatt_conn_count;
}

///////////////////////////////////////////////////////////////////////////////////
// Função: getConnectionsAvailables();
// Descrição: Retorna retorna o ultimo indice de conexão disponível
// Argumentos: void 
// Retorno:    gatt_coon_count -> [-1 0 .. GATT_GATTS_NUM_CONN_MAX-1]
// OBs: -1 indica que não existem conexões disponíveis 
//////////////////////////////////////////////////////////////////////////////////
int8_t gattuartRegisterDevice(uint8_t deviceIndx){
    int16_t connAvailable = -1;
    // Encontrar links disponiveis(Exceto celular)
    for(uint8_t i=0;i<GATTS_NUM_CONN_MAX;i++){
        if(freeConection[i] == 1){
            connAvailable = i;
            break;
        }
    }

    if(connAvailable == -1)
        return connAvailable;

    // Atribuir connID ao device
    deviceGet(deviceIndx)->gattsConnectionID = connAvailable;
    // Conexão não disponivel
    freeConection[connAvailable] =0;
    

    return connAvailable;
}

int8_t gattuartUnregisterDevice(uint16_t connID){
   //  ESP_LOGI("GATTS_UART","Disconected ConnID: %d",connID);
    for(uint8_t i=0; i< DEVICE_QNT-1;i++){
        if(deviceGet(i)->connID == connID){
            freeConection[i] = 0;
            deviceGet(i)->isConnected=0;
            deviceGet(i)->gattsConnectionID = -1;
            ESP_LOGI("GATTS_UART","Disconected Device: %d",i);
            return connID;
        }
    }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////////
//Implementations - Funções get e set - INICIO
////////////////////////////////////////////////////////////////////////////////////

uint8_t bleuartServerGetScanRspConfig(){
    return scan_rsp_config_flag;
}

uint8_t bleuartServerGetAdvConfigDone(){
    return adv_config_done;
}

void bleuartServerSetAdvConfigDone(uint8_t newStatus){
    adv_config_done = newStatus;
}

esp_ble_adv_params_t * bleuartServerGetAdvParam(){
    return &adv_params;
}