#include "mqtts_eth.h"
#include "eth.h"
#include "communication.h"
#include "uart.h"
#include "bleuartServer.h"
#include "blemanager.h"
#include "device.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi.h"
#include "esp_log.h"


TaskHandle_t check_messages_task_handler;
TaskHandle_t publish_messages_task_handler;
TaskHandle_t commUpdateBufferTask_handler;
TaskHandle_t commUpdateRedeTask_handler;
TaskHandle_t deviceConnTask_handler;

void init_main(void){

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI("NVS", "ERRO AO ABRIR O NVS");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE("WIFI MAC", "Erro ao inicializar o Wi-Fi: %s", esp_err_to_name(err));
        return;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE("WIFI MAC", "Erro ao iniciar o Wi-Fi: %s", esp_err_to_name(err));
        return;
    }

    uint8_t mac[6];
    err = esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    if (err != ESP_OK) {
        ESP_LOGE("WIFI MAC", "Erro ao obter o endereço MAC: %s", esp_err_to_name(err));
        return;
    }

    // Formatar e imprimir o endereço MAC
    char macAddress[30];
    snprintf(macAddress, sizeof(macAddress), "arcelor/%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ESP_LOGI("WIFI MAC", "Endereço MAC Wi-Fi: %s", macAddress);

    // Encerrar a conexão Wi-Fi
    err = esp_wifi_stop();
    if (err != ESP_OK) {
        ESP_LOGE("WIFI MAC", "Erro ao encerrar o Wi-Fi: %s", esp_err_to_name(err));
        return;
    }

    set_mac_variable(macAddress);
    uartInit(UART2_INSTANCE);
    initialize_comunication();
    initialize_ethernet();
    initialize_mqtts();
    bleManagerInit();
    bleuartServerInit(0x0010);

    vTaskDelay(2000/portTICK_PERIOD_MS);
}

void app_main(void){
    init_main();
    
    xTaskCreatePinnedToCore(check_messages_task,"CHECK_MESSAGES",(1024 * 10),NULL,0,&check_messages_task_handler,0);
    configASSERT(check_messages_task_handler);

    xTaskCreatePinnedToCore(publish_messages_task,"PUBLISH_MESSAGES",(1024 * 10),NULL,0,&publish_messages_task_handler,0);
    configASSERT(publish_messages_task_handler);

    xTaskCreatePinnedToCore(commUpdateBufferTask,"BUFFER",(1024 * 10),NULL,0,&commUpdateBufferTask_handler,0);
    configASSERT(commUpdateBufferTask_handler);

    xTaskCreatePinnedToCore(commUpdateRedeTask,"CHECK_REDE",(1024 * 10),NULL,0,&commUpdateRedeTask_handler,0);
    configASSERT(commUpdateRedeTask_handler);

    // Cria task para realizar a checagem de conexão dos dispositivos BLE (Perfiérico + UART) //500ms  300 
    xTaskCreatePinnedToCore(deviceConnectionTask,"DEVICE",(1024 *10),NULL,1,&deviceConnTask_handler,0);
    configASSERT(deviceConnTask_handler);
}