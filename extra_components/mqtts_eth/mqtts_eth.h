#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "communication.h"
#include "esp_wifi.h"
#include "esp_timer.h"

void initialize_mqtts(void);
void check_messages_task(void *pvParameter);
void publish_messages_task(void *pvParameter);
void set_mac_variable(char *mac);
void commDisconnectedTask(void *pvParameter);
void publish_mqtts(uint8_t id, char *ip, char *netmask, char *gateway, char *dns);