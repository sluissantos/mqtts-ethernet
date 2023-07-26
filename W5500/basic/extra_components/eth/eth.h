#pragma once
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/spi_master.h"
#include "ping/ping_sock.h"
#include "sys/socket.h"
#include "netdb.h"
#include "esp_ping.h"

/// @brief 
void initialize_ethernet();
/// @brief 
/// @return 
bool ip_obtained();

