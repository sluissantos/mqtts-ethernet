#include "ping_main.h"
#include "mqtts_main.h"
#include "eth_main.h"

void app_main(void){

    initialize_ethernet();

    while (ip_obtained()==false) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    initialize_mqtts();

    while (1){
        publish_mqtts();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}