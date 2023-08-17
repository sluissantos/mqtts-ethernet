#include "mqtts_eth.h"
#include "eth.h"
#include "communication.h"
#include "uart.h"
#include "bleuartServer.h"
#include "blemanager.h"
#include "device.h"
#include "nvs_flash.h"
#include "nvs.h"

TaskHandle_t check_messages_task_handler;
TaskHandle_t publish_messages_task_handler;
TaskHandle_t commUpdateBufferTask_handler;
TaskHandle_t commUpdateRedeTask_handler;
TaskHandle_t deviceConnTask_handler;

uint8_t id_display = 1;

void init_main(void){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI("NVS", "ERRO AO ABRIR O NVS");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    uartInit(UART2_INSTANCE);
    initialize_comunication(id_display);
    initialize_ethernet();
    initialize_mqtts();
    bleManagerInit();
    bleuartServerInit(0x0010);
}

void app_main(void){
    init_main();
    
    xTaskCreatePinnedToCore(check_messages_task,"CHECK_MESSAGES",(1024 * 5),NULL,0,&check_messages_task_handler,0);
    configASSERT(check_messages_task_handler);

    xTaskCreatePinnedToCore(publish_messages_task,"PUBLISH_MESSAGES",(1024 * 5),NULL,0,&publish_messages_task_handler,0);
    configASSERT(publish_messages_task_handler);

    xTaskCreatePinnedToCore(commUpdateBufferTask,"BUFFER",(1024 * 5),NULL,0,&commUpdateBufferTask_handler,0);
    configASSERT(commUpdateBufferTask_handler);

    xTaskCreatePinnedToCore(commUpdateRedeTask,"CHECK_REDE",(1024 * 5),NULL,0,&commUpdateRedeTask_handler,0);
    configASSERT(commUpdateRedeTask_handler);

    // Cria task para realizar a checagem de conexão dos dispositivos BLE (Perfiérico + UART) //500ms  300 
    xTaskCreatePinnedToCore(deviceConnectionTask,"DEVICE",(1024 *5),NULL,1,&deviceConnTask_handler,0);
    configASSERT(deviceConnTask_handler);

   vTaskDelay(2000/portTICK_PERIOD_MS);
}