#include "blemanager.h"

#include "bleuartServer.h"

#define BLEMAN_TAG "BleManager: "

static bool Isconnecting    = false;
static bool stop_scan_done  = false;

static void bluetoothGapEventHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

///////////////////////////////////////////////////////////////////////////////////
// Função: bleManagerInit();
// Descrição: Função responsável por iniciar adaptador bluetooth e stack bluedroid.
// Obs: Essa função também inicializa a função gap_callback() (Generic Acess Profile)
// Argumentos: void
// Retorno:    void
//////////////////////////////////////////////////////////////////////////////////
void bleManagerInit(){
    esp_err_t ret;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(BLEMAN_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(BLEMAN_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(BLEMAN_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(BLEMAN_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    
    ret = esp_ble_gap_register_callback(bluetoothGapEventHandler);
    if (ret){
        ESP_LOGE(BLEMAN_TAG, "gap register error, error code = %x", ret);
        return;
    }
     esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
       // ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
    esp_power_level_t power;
    power = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_CONN_HDL0);
    ESP_LOGI(BLEMAN_TAG, "Tx Connection0 Power: %d", power);
    
    power = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_ADV);
    ESP_LOGI(BLEMAN_TAG, "Tx Adv Power: %d", power);

    power = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_SCAN);
    ESP_LOGI(BLEMAN_TAG, "Tx Scan Power: %d", power);
}

///////////////////////////////////////////////////////////////////////////////////
// Função: bluetoothGapEventHandler();
// Descrição: Função responsável por tratar os eventos GAP.
// Argumentos: void
// Retorno:    void
//////////////////////////////////////////////////////////////////////////////////
static void bluetoothGapEventHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param){
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;
   
    switch (event) {
    // Server events 
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:{
        // adv_config_done &= (~adv_config_flag);
         bleuartServerSetAdvConfigDone(bleuartServerGetAdvConfigDone()&(~bleuartServerGetAdvConfigDone()));
        if (bleuartServerGetAdvConfigDone() == 0){
            esp_ble_gap_start_advertising(bleuartServerGetAdvParam()/*&adv_params*/);
        }
    }   
    break;

    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:{
        //adv_config_done &= (~scan_rsp_config_flag);
        bleuartServerSetAdvConfigDone(bleuartServerGetAdvConfigDone()&(~bleuartServerGetScanRspConfig()));
        if (bleuartServerGetAdvConfigDone() == 0){
            esp_ble_gap_start_advertising(bleuartServerGetAdvParam());
        }
    }
    break;

    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:{
        //advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(BLEMAN_TAG, "Advertising start failed\n");
        }
    }
    break;

    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:{

    }
    break;

    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:{
        if(param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS){
            ESP_LOGI(BLEMAN_TAG,"Scan started!");
            esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN,ESP_PWR_LVL_P9);
            ESP_LOGI(BLEMAN_TAG,"Scan Power %d",esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_SCAN));
        }else{
            ESP_LOGI(BLEMAN_TAG,"Scan start fail!");
        }
    }
    break;

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:{
     if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
         ESP_LOGE(BLEMAN_TAG, "Scan stop failed");
         break;
      }
     ESP_LOGI(BLEMAN_TAG, "Stop scan successfully");
    }
    break;

    // Connection events
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:{
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(BLEMAN_TAG, "Advertising stop failed\n");
        } else {
            ESP_LOGI(BLEMAN_TAG, "Stop adv successfully\n");
        }
    }
    break;

    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:{
    ESP_LOGI("GATTS_TAG", "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
    }
    break;   

    default:
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////////
// Função: bleManagerInitBleServerUart();
// Descrição: Inicia o Gatt server para serviço Nordic Uart
// Argumentos: void
// Retorno:    void
//////////////////////////////////////////////////////////////////////////////////
void bleManagerInitBleServerUart(uint16_t ID){
    bleuartServerInit(ID);
}