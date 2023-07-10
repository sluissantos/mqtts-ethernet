#include "mqtts_main.h"

//#define EXAMPLE_BROKER_URI "mqtt://gwqa.revolog.com.br:1884"
#define EXAMPLE_BROKER_URI "mqtt://192.168.15.61:1883"
//#define EXAMPLE_BROKER_URI "mqtts://192.168.15.61:8883"
#define MQTT_TOPIC "test-ethernet/message"
#define QOS 2
static const char *TAG ="MQTTS";

extern const uint8_t broker_cert_pem_start[] asm("_binary_broker_ca_pem_start");
extern const uint8_t broker_cert_pem_end[] asm("_binary_broker_ca_pem_end");

static esp_mqtt_client_handle_t client;
int msg_id;
static bool flag_connected = false;
static bool flag_subscribed = false;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event){
        // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            if(flag_subscribed==false){
                    msg_id = esp_mqtt_client_subscribe(client, MQTT_TOPIC, QOS);
                    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
                    flag_subscribed = true;
            }
            flag_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            flag_connected = false;
            flag_subscribed = false;
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            ESP_LOGI(TAG, "entrou no teste");
            flag_subscribed = true;
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            flag_subscribed = false;
            flag_connected = false;
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_app_start(void){
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = EXAMPLE_BROKER_URI,
        .event_handle = mqtt_event_handler,
        //.cert_pem =  (const char *)broker_cert_pem_start,
        //.username = "tecnologia",
        //.password = "128Parsecs!",
        .username = "",
        .password = "",
        .lwt_topic = MQTT_TOPIC,
        //.lwt_msg = "{\"alive\":\"on\"}",
        //.lwt_msg_len= strlen("{\"alive\":\"on\"}"),
        .lwt_qos = QOS,
        .lwt_retain = 0,
        .keepalive = 60
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

void publish_mqtts(){
    if(flag_connected){
        msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC, "ferias", 0, QOS, 1);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
    }
    else{
        msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC, "ferias", 0, QOS, 1);
        ESP_LOGI(TAG, "Not published.Client not connected.");
    }
}

void initialize_mqtts(void){
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
    mqtt_app_start();
}