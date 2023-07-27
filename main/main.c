#include "ping.h"
#include "mqtts_eth.h"
#include "eth.h"
#include "comunication.h"
#include "uart.h"

TaskHandle_t check_messages_task_handler;
TaskHandle_t commUpdateBufferTask_handler;

void init_main(void){
    uartInit(UART2_INSTANCE);
    initialize_ethernet();
}

void app_main(void){
    init_main();

    while (ip_obtained()==false) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    initialize_mqtts();

    xTaskCreatePinnedToCore(check_messages_task,"CHECK_MESSAGES",(1024 * 5),NULL,0,&check_messages_task_handler,0);
    configASSERT(check_messages_task_handler);

    xTaskCreatePinnedToCore(commUpdateBufferTask,"BUFFER",(1024 * 5),NULL,0,&commUpdateBufferTask_handler,0);
    configASSERT(commUpdateBufferTask_handler);
    
    commSendHexDataWithDelay();
    /*
    while (1){
        publish_mqtts("test/status");
        vTaskDelay(20000 / portTICK_PERIOD_MS);
    }
    */
    
}