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
#include "nvs_flash.h"
#include "nvs.h"
#include "communication.h"
#include "mqtts_eth.h"

#define IP "ip"
#define GATEWAY "gateway"
#define NETMASK "netmask"
#define DNS "dns"

void initialize_ethernet(const uint8_t mac[6]);
void ip_obtained(void);
void change_rede(char *ip, char *gateway, char *netmask, char *dns);
void nvs_erase(void);
void change_ip(char *ip);
void change_gateway(char *gateway);
void change_netmask(char *netmask);
void change_dns(char *dns);
void store_ethernet_one_variable(char *ident, char *param);
char* retrieve_ethernet_one_variable(char *variable);
void change_one_rede_variable(char* ident, char *param);
