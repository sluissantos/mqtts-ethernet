#include "mqtts_eth.h"

#define TIMESLEEP 5000000 // 5 segundos
#define QOS 1

//#define EXAMPLE_BROKER_URI "mqtt://192.168.15.5:1883"
//#define EXAMPLE_BROKER_URI "mqtts://192.168.15.4:8883"

static const char *TAG ="MQTTS";

extern const uint8_t broker_cert_pem_start[] asm("_binary_broker_ca_pem_start");
extern const uint8_t broker_cert_pem_end[] asm("_binary_broker_ca_pem_end");

static esp_mqtt_client_handle_t client;
static bool flag_connected = false;
static bool flag_subscribed = false;
static bool new_message = false;

char broker_uri[256] = "mqtt://gwqa.revolog.com.br:1884";
char user[256] = "tecnologia";
char password[256] = "128Parsecs!";
char* namespace_mqtt = "namespace_mqtt";

static char* publish_rede = "arcelor/status/rede";
static char* publish_message = "arcelor/status/message";
static char* subscribe_topic_message = "arcelor/message";
static char* subscribe_topic_rede = "arcelor/rede";
static char* subscribe_topic_mac;

static char* payload;
static char* last_message;

uint8_t cont = 0;

void publish_status_rede(uint8_t id, char *ip, char *gateway, char *netmask, char *dns){
    if(flag_connected){
        char *json_str = NULL;
        cJSON *status_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(status_json, "tmst", esp_timer_get_time()/10000);
        cJSON_AddNumberToObject(status_json, "id", id);
        cJSON_AddStringToObject(status_json, "ip", ip);
        cJSON_AddStringToObject(status_json, "gateway", gateway);
        cJSON_AddStringToObject(status_json, "netmask", netmask);
        cJSON_AddStringToObject(status_json, "dns", dns);    
        json_str = cJSON_Print((const cJSON *)status_json);

        esp_mqtt_client_publish(client, publish_rede, (char *)json_str, strlen(json_str), QOS, 0);
        free(json_str);
        cJSON_Delete(status_json);
    }
    return;
}

void publish_status_message(char *message){
    if (flag_connected && message != NULL) {
        esp_mqtt_client_publish(client, publish_message, message, strlen(message), QOS, 0);
    }
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

void save_last_message(){
    if(last_message != NULL){
        store_ethernet_one_variable(LAST_MESSAGE, last_message);
    }
}

void commDisconnectedTask(void *pvParameter){
    while (1){
        if(flag_connected == false){
            cont++;
            if(cont == 3){
                set_message_offline();
            }
            if(cont == 6){
                if(last_message != NULL){
                    save_last_message();
                }
                eth_reset();
            }
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            if(flag_subscribed==false){
                    esp_mqtt_client_subscribe(client, subscribe_topic_message, QOS);
                    ESP_LOGI(TAG, "SENT SUBSCRIBE MESSAGE SUCCESSFULL%d", event->msg_id);
                    esp_mqtt_client_subscribe(client, subscribe_topic_rede, QOS);
                    ESP_LOGI(TAG, "SENT SUBSCRIBE REDE SUCCESSFULL%d", event->msg_id);
                    esp_mqtt_client_subscribe(client, subscribe_topic_mac, QOS);
                    ESP_LOGI(TAG, "SENT SUBSCRIBE MAC SUCCESSFULL%d", event->msg_id);
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
            if (strncmp(subscribe_topic_message, event->topic, strlen(subscribe_topic_message)) == 0) {
                cJSON *json_obj = cJSON_ParseWithLength(event->data, event->data_len);
                if (json_obj != NULL) {
                    char *json_str = cJSON_Print(json_obj);
                    if (json_str != NULL) {
                        if (last_message != NULL) {
                            free(last_message);
                        }
                        last_message = json_str;
                        cJSON_Delete(json_obj);
                        publish_status_message(last_message);
                    }
                }
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
        .uri = broker_uri,
        //.cert_pem =  (const char *)broker_cert_pem_start,
        .username = user,
        .password = password,
        .lwt_qos = QOS
    };

    memset(&client,0,sizeof(esp_mqtt_client_handle_t));
    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    if(client){
        esp_mqtt_client_start(client);
    }
    return;
}

void set_mac_variable(char *mac){
    subscribe_topic_mac = strdup(mac);
}

char* retrieve_broker_one_variable(char *variable) {
    ESP_LOGI("ENTROU", "ENTROU AQUI");

    nvs_handle_t mqtt_nvs_handle;

    esp_err_t err = nvs_open(namespace_mqtt, NVS_READONLY, &mqtt_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error to open namespace_mqtt to retrieve");
        return NULL;
    }

    char buffer[512]; // Buffer para armazenar as strings recuperadas
    char *var = NULL;

    // Recuperar as variáveis
    size_t required_size;
    err = nvs_get_str(mqtt_nvs_handle, variable, NULL, &required_size);
    if (err == ESP_OK) {
        if (required_size <= sizeof(buffer)) {
            err = nvs_get_str(mqtt_nvs_handle, variable, buffer, &required_size);
            if (err == ESP_OK) {
                var = strdup(buffer); // Aloca memória e copia o valor
                ESP_LOGE("retrieve", "variable: %s", variable);
            }
            else {
                ESP_LOGI(TAG, "Error to retrieve variable");
            }
        }
    }
    // Fechar o namespace NVS
    nvs_close(mqtt_nvs_handle);

    return var;
}

void store_broker_one_variable(char *ident, char *param){
    nvs_handle_t mqtt_nvs_handle;

    esp_err_t err = nvs_open(namespace_mqtt, NVS_READWRITE, &mqtt_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening namespace_mqtt: %s", esp_err_to_name(err));
        return;
    }

    if(param != NULL){
        err = nvs_set_str(mqtt_nvs_handle, ident, param);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error to store %s, %s", ident, esp_err_to_name(err));
        }
        else{
            ESP_LOGE("store", "%s: %s", ident, param);
        }
    }

    // Efetivamente escrever as alterações no armazenamento permanente
    err = nvs_commit(mqtt_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS %s changes: %s", ident, esp_err_to_name(err));
    }

    nvs_close(mqtt_nvs_handle);
    esp_restart();
}

void initialize_mqtts(){
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
    ESP_ERROR_CHECK(nvs_flash_init());

    char retrieved_broker_param[256];
    if(retrieve_broker_one_variable(BROKER_URI) != NULL){
        strcpy(retrieved_broker_param, retrieve_broker_one_variable(BROKER_URI));
        memcpy(broker_uri, retrieved_broker_param, strlen(retrieved_broker_param) + 1); // Copia a string, incluindo o caractere nulo de terminação
    }

    if(retrieve_broker_one_variable(USER) != NULL){
        strcpy(retrieved_broker_param, retrieve_broker_one_variable(USER));
        memcpy(user, retrieved_broker_param, strlen(retrieved_broker_param) + 1); // Copia a string, incluindo o caractere nulo de terminação
    }

    if(retrieve_broker_one_variable(PASSWORD) != NULL){
        strcpy(retrieved_broker_param, retrieve_broker_one_variable(PASSWORD));
        memcpy(password, retrieved_broker_param, strlen(retrieved_broker_param) + 1); // Copia a string, incluindo o caractere nulo de terminação
    }

    mqtt_app_start();
    return;
}
