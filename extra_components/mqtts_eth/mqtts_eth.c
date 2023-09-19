#include "mqtts_eth.h"

#define QOS 1
#define EXAMPLE_BROKER_URI "mqtt://gwqa.revolog.com.br:1884"
//#define EXAMPLE_BROKER_URI "mqtt://192.168.15.176:1883"
//#define EXAMPLE_BROKER_URI "mqtts://192.168.15.4:8883"

static const char *TAG ="MQTTS";

extern const uint8_t broker_cert_pem_start[] asm("_binary_broker_ca_pem_start");
extern const uint8_t broker_cert_pem_end[] asm("_binary_broker_ca_pem_end");

static esp_mqtt_client_handle_t client;
static bool flag_connected = false;
static bool flag_subscribed = false;
static bool new_message = false;
static char* payload;
static char* publish_status = "arcelor/status";
static char* subscribe_topic_message = "arcelor/message";
static char* subscribe_topic_rede = "arcelor/rede";
static char* subscribe_topic_mac;
char status_message[40];

void publish_mqtts(uint8_t id, char *ip, char *gateway, char *netmask, char *dns){
    if(flag_connected){
        char *json_str = NULL;
        cJSON *status_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(status_json, "tmst", esp_timer_get_time()/1000);
        cJSON_AddNumberToObject(status_json, "id", id);
        cJSON_AddStringToObject(status_json, "ip", ip);
        cJSON_AddStringToObject(status_json, "gateway", gateway);
        cJSON_AddStringToObject(status_json, "netmask", netmask);
        cJSON_AddStringToObject(status_json, "dns", dns);    
        json_str = cJSON_Print((const cJSON *)status_json);

        esp_mqtt_client_publish(client, publish_status, (char *)json_str, strlen(json_str), QOS, 1);
        free(json_str);
        cJSON_Delete(status_json);
    }
    return;
}

void check_messages_task(void *pvParameter){
    while (1){
        if (new_message){
            set_variables(payload);
            free(payload);
            new_message = false;
        }
		taskYIELD();
    }
}

void commDisconnectedTask(void *pvParameter){
    uint16_t cont = 0;
    while (1){
        if(flag_connected == false){
            cont++;
            if(cont == 6){
                esp_restart();
            }
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}


static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event){
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            if(flag_subscribed==false){
                    esp_mqtt_client_subscribe(client, subscribe_topic_message, QOS);
                    ESP_LOGI(TAG, "SENT SUBSCRIBE MESSAGE SUCCESSFULL");
                    esp_mqtt_client_subscribe(client, subscribe_topic_rede, QOS);
                    ESP_LOGI(TAG, "SENT SUBSCRIBE REDE SUCCESSFULL");
                    esp_mqtt_client_subscribe(client, subscribe_topic_mac, QOS);
                    ESP_LOGI(TAG, "SENT SUBSCRIBE MAC SUCCESSFULL");
                    flag_subscribed = true;
            }
            flag_connected = true;
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            flag_connected = false;
            flag_subscribed = false;
            esp_mqtt_client_start(client); // Tente reconectar
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED");
            flag_subscribed = true;
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED");
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
                if(event->data != NULL){
                    payload = (char *)malloc(strlen(event->data) + 1);
                    strcpy(payload, event->data);
                    new_message = true;
                }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            flag_subscribed = false;
            flag_connected = false;
            esp_mqtt_client_start(client); // Tente reconectar
            break;

        default:
            flag_subscribed = false;
            flag_connected = false;
            esp_mqtt_client_start(client); // Tente reconectar
            ESP_LOGI(TAG, "OTHER EVENTS ID:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_app_start(void){
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = EXAMPLE_BROKER_URI,
        .event_handle = mqtt_event_handler,
        //.cert_pem =  (const char *)broker_cert_pem_start,
        .username = "tecnologia",
        .password = "128Parsecs!",
        .keepalive = 0
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);


    return;
}

void set_mac_variable(char *mac){
    subscribe_topic_mac = strdup(mac);
}

void initialize_mqtts(){
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    mqtt_app_start();
    return;
}