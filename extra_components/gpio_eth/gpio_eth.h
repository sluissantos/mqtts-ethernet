#ifndef GPIO_ETH_H
#define GPIO_ETH_H

#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_event.h"

#define W5500_RESET_PIN GPIO_NUM_21

void init_gpio();
void eth_reset();

#endif