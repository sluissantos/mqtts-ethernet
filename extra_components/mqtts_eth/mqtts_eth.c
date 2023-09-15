#include "mqtts_eth.h"

#define QOS 2
#define EXAMPLE_BROKER_URI "mqtt://gwqa.revolog.com.br:1884"
//#define EXAMPLE_BROKER_URI "mqtt://192.168.15.176:1883"
//#define EXAMPLE_BROKER_URI "mqtts://192.168.15.4:8883"

static char* publish = "arcelor/status";
static char* subscribe_message = "arcelor/message";
static char* subscribe_rede = "arcelor/rede";

static const char *TAG ="MQTTS";

extern const uint8_t broker_cert_pem_start[] asm("_binary_broker_ca_pem_start");
extern const uint8_t broker_cert_pem_end[] asm("_binary_broker_ca_pem_end");

static esp_mqtt_client_handle_t client;
static bool flag_connected = false;
static bool flag_subscribed = false;
static bool new_message = false;
static char* payload;
static char* publish_topic;
static char* subscribe_topic_message;
static char* subscribe_topic_rede;
static char* subscribe_topic_mac;
uint8_t id;
char status_message[40];

void set_id(uint8_t ident){
    id = ident;
}

void publish_mqtts(){
    if(flag_connected){
        sprintf(status_message,"\"display_id\":%d\n\"status\":\"ok\"", id);
        esp_mqtt_client_publish(client, publish_topic, status_message, 0, QOS, 1);
    }
    return;
}

void publish_messages_task(){
    while (1){
        publish_mqtts(publish_topic);
        //ESP_LOGI(TAG, "MQTT PUBLISH");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

void check_messages_task(){
    while (1){
        if (new_message){
            set_variables(payload);
            free(payload);
            new_message = false;
        }
		taskYIELD();
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
                    ESP_LOGI("TESTE", "tamanho=%d", event->topic_len);
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
    publish_topic = strdup(publish);
    subscribe_topic_message = strdup(subscribe_message);
    subscribe_topic_rede = strdup(subscribe_rede);

    mqtt_app_start();
    return;
}