#include "communication.h"

#define COMM_BUFFER_SIZE 1024
#define COMM_TASK_DELAY  50  //ms
#define COMM_DISTANCE_VERB 	 1
#define COMM_FORK_PARAM_VERB 2

uint8_t txBuffer[COMM_BUFFER_SIZE] = {0};
uint8_t rxBuffer[COMM_BUFFER_SIZE] = {0};
// Buffer circular uart
uint8_t uartRxBuffer[COMM_BUFFER_SIZE] = {0};
uint8_t estadoPacote = 0; 
uint8_t CusorEscritaPacote = 0;
uint8_t TamanhoNovoPacote = 0;
uint8_t lengthDataUart = 0;

static const char *TAG ="COMMUNICATION";
char *name_space = "comm_namespace";

uint8_t id_default = 1;

uint8_t id_nvs;

uint8_t define_id = 3;
uint8_t id=3;
uint8_t id_display;
uint8_t left=2;
uint8_t right=2;

char* plate;

char *ip_c;
char *gateway_c;
char *netmask_c; 
char *dns_c;
uint8_t flag_erase=0;

uint8_t* data = 0;
size_t dataLen;
bool status = false;
bool status_rede = false;
bool status_id = false;

void parse_json(const char* jsonString) {
    cJSON* json = cJSON_Parse(jsonString);
    if (json == NULL) {
        printf("Erro ao fazer o parse do JSON.\n");
        return;
    }

    cJSON* restart_object = cJSON_GetObjectItem(json, "restart");
    if (restart_object != NULL) {
        if(restart_object->valueint == 1){
            save_last_message();
            esp_restart();
        }
    }

    cJSON* reset_object = cJSON_GetObjectItem(json, "reset");
    if (reset_object != NULL) {
        if(reset_object->valueint == 1){
            eth_reset();
        }
    }
    
    cJSON* id_object = cJSON_GetObjectItem(json, "id");
    if (id_object != NULL) {
        id = id_object->valueint;
    } else {
        id = 3;
    }

    cJSON* left_object = cJSON_GetObjectItem(json, "left");
    if (left_object != NULL) {
        left = left_object->valueint;
    } else {
        left = 2;
    }

    cJSON* right_object = cJSON_GetObjectItem(json, "right");
    if (right_object != NULL) {
        right = right_object->valueint;
    } else {
        right = 2;
    }

    cJSON* plate_object = cJSON_GetObjectItem(json, "plate");
    if (plate_object != NULL) {
        plate = (char *)malloc(strlen(plate_object->valuestring) + 1);
        strcpy(plate, plate_object->valuestring);
    } else {
        plate = NULL;
    }

    // Acessa diretamente a chave "data" no JSON
    cJSON *data_object = cJSON_GetObjectItem(json, "data");
    if (data_object != NULL && cJSON_IsArray(data_object)) {
        dataLen = cJSON_GetArraySize(data_object);
        data = (uint8_t *)malloc(dataLen * sizeof(uint8_t));

        // Copia os valores do JSON para data_array
        for (size_t i = 0; i < dataLen; ++i) {
            cJSON *value = cJSON_GetArrayItem(data_object, i);
            if (value != NULL && cJSON_IsNumber(value)) {
                data[i] = (uint8_t)value->valueint;
            }
        }

    } else {
        ESP_LOGI("COMM","Chave 'data' não encontrada no JSON.\n");
        data = NULL;
    }

    //mensagens de setup de rede
    cJSON* ip_object = cJSON_GetObjectItem(json, "ip");
    if (ip_object != NULL) {
        ip_c = (char *)malloc(strlen(ip_object->valuestring) + 1);
        strcpy(ip_c, ip_object->valuestring);
    } else {
        ip_c = NULL;
    }

    cJSON* gateway_object = cJSON_GetObjectItem(json, "gateway");
    if (gateway_object != NULL) {
        gateway_c = (char *)malloc(strlen(gateway_object->valuestring) + 1);
        strcpy(gateway_c, gateway_object->valuestring);
    } else {
        gateway_c = NULL;
    }


    cJSON* netmask_object = cJSON_GetObjectItem(json, "netmask");
    if (netmask_object != NULL) {
        netmask_c = (char *)malloc(strlen(netmask_object->valuestring) + 1);
        strcpy(netmask_c, netmask_object->valuestring);
    } else {
        netmask_c = NULL;
    }


    cJSON* dns_object = cJSON_GetObjectItem(json, "dns");
    if (dns_object != NULL) {
        dns_c = (char *)malloc(strlen(dns_object->valuestring) + 1);
        strcpy(dns_c, dns_object->valuestring);
    } else {
        dns_c = NULL;
    }

    cJSON* erase_object = cJSON_GetObjectItem(json, "erase");
    if (erase_object != NULL) {
        flag_erase = erase_object->valueint;
    } else {
        flag_erase = 0;
    }

    cJSON* define_id_object = cJSON_GetObjectItem(json, "define");
    if (define_id_object != NULL) {
        define_id = define_id_object->valueint;
        status_id = true;
    } else {
        define_id = 3;
    }

    if(ip_c != NULL || gateway_c != NULL || netmask_c != NULL || dns_c != NULL || flag_erase == 1){
        status_rede = true;
    }

    // Após usar o JSON, lembre-se de liberar a memória ocupada por ele.
    cJSON_Delete(json);
	return;
}

void set_variables(char *payload){
	parse_json(payload);
	status = true;
	return;
}

///////////////////////////////////////////////////////////////////////////////////
// Função: commSendDataInterface(uint8_t *Dado, uint8_tam);
// Descrição: Envia dado para dispositivo selecionado
// Argumentos: uint8_t interface -> Dispositivo para envio de dados
// 			   uint8_t *Dado   -> ponteiro contendo os dados
// 			   uint8_t length  -> Tamanho dos dados
//////////////////////////////////////////////////////////////////////////////////
void commSendDataInterface(uint8_t *Dado, uint8_t length) {
    uartSendData(UART2_INSTANCE, (char *)Dado, length);
}

void send_data(uint8_t *data, uint8_t dataSize) {
    if (data != NULL) {
        for (uint8_t i = 0; i < dataSize; i++) {
            commSendDataInterface(&data[i], 1);
            ESP_LOGI("COMM", "data[%d] = 0x%X", i, data[i]);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    } else {
        ESP_LOGI("COMM", "data is null");
    }
}


///////////////////////////////////////////////////////////////////////////////////
// Função: commUpdateBufferTask(uint16_t );
// Descrição: Define nova de decawaveID 
// Argumentos: uint8_t deviceIndx -> DEVICE_UART_INDX
//             uint8_t length -> Tamanho do vetor data + 1 devido ao OPCODE
// Retorno:    device_t -> Struct contendo dados do device
//////////////////////////////////////////////////////////////////////////////////
void commUpdateBufferTask(void *pvParameter) {
   while (1){
        uint8_t data_stop[] = {0x00, 0x96, 0x03, 0xFF, 0XC5, 0XC5, 0x1F, 0x62, 0x11};

        uint8_t data_left[] = {0x00, 0x96, 0x03, 0xFF, 0xC5, 0xC5, 0x1F, 0x12, 0x1F, 0x12, ' ', ' ', ' ', ' ', ' ', ' ', 0x11};

        uint8_t data_right[] = {0x00, 0x96, 0x03, 0xFF, 0xC5, 0xC5, 0x7E, ' ', ' ', ' ', ' ', ' ', 0x1F, 0x0E, 0x1F, 0x0E, 0x11};

        uint8_t data_id_left[] = {0x00, 0x96, 0x03, 0xFF, 0xC5, 0xC5, 0x01, ' ', ' ', ' ', ' ', ' ', ' ', ' ',
         0xFF, 0xC5, 0xC5, 0x01, ' ', ' ', ' ', ' ', ' ', ' ', ' ',
            0xFF, 0xC0, 0xC0, 0x1F, 0x12, 0x1F, 0x12, 0x1F, 0x12, 0x1F, 0x12, 0x1F, 0x12, 0x1F, 0x12,
                0x1F, 0x12, 0x1F, 0x12, 0x1F, 0x12, 0x1F, 0x12, 0x1F, 0x12, 0x1F, 0x12, 0x11};

        uint8_t data_id_right[] = {0x00, 0x96, 0x03, 0xFF, 0xC5, 0xC5, 0x01, ' ', ' ', ' ', ' ', ' ', ' ', ' ',
         0xFF, 0xC5, 0xC5, 0x01, ' ', ' ', ' ', ' ', ' ', ' ', ' ',
            0xFF, 0xC1, 0xC1, 0x1F, 0x0E, 0x1F, 0x0E, 0x1F, 0x0E, 0x1F, 0x0E,0x1F, 0x0E, 0x1F, 0x0E, 
                0x1F, 0x0E, 0x1F, 0x0E,0x1F, 0x0E, 0x1F, 0x0E, 0x1F, 0x0E, 0x1F, 0x0E, 0x11};

        if (status) {
            ESP_LOGI("TESTE", "ENTROU AQUI");
            if(id == 0 && id_display == 0) {
                if(data != NULL){
                    send_data(data, dataLen);
                }
                else if(left == 1 && right == 0){
                    if(strlen(plate) == 0){
                        send_data(data_stop, sizeof(data_stop));
                    }
                    else {
                        for (uint8_t i = 0; i < strlen(plate) && strlen(plate)<=6; i++) {
                            data_left[10 + i] = plate[i];
                        }
                        send_data(data_left, sizeof(data_left));
                    }
                }
                else if(left == 0 && right == 1){
                    if(strlen(plate) == 0){
                        send_data(data_stop, sizeof(data_stop));
                    }
                    else {
                        uint8_t i,j;
                        for (i = 6-strlen(plate), j = 0; i < 6; i++, j++) {
                            data_right[6+i] = plate[j];
                        }
                        send_data(data_right, sizeof(data_right));
                    }
                }
                else{
                    ESP_LOGI("COMM","ERRO NA ORIENTACAO");
                }
            }

            else if(id == 1 && id_display == 1) {
                if(data != NULL){
                    send_data(data, dataLen);
                }
                else if(strlen(plate) == 0){
                    send_data(data_stop, sizeof(data_stop));
                }

                else if(left == 1 && right ==0){
                    for (uint8_t i = 0; i < 7; i++) {
                        data_id_left[7 + i] = plate[i];
                        data_id_left[18 + i] = plate[i];
                    }
                    send_data(data_id_left, sizeof(data_id_left));
                }

                else if(left == 0 && right == 1){
                    for (uint8_t i = 0; i < 7; i++) {
                        data_id_right[7 + i] = plate[i];
                        data_id_right[18 + i] = plate[i];
                    }
                    send_data(data_id_right, sizeof(data_id_right));
                }

                else{
                    ESP_LOGI("COMM","ERRO NA ORIENTACAO");
                }
                
            }

            else if(id == 2 && id_display == 2) {
                if(data != NULL){
                    send_data(data, dataLen);
                }  		
                else if(left == 1 && right == 0){
                    if(strlen(plate) == 0){
                        send_data(data_stop, sizeof(data_stop));
                    }
                    else {
                        for (uint8_t i = 0; i < strlen(plate) && strlen(plate)<=6; i++) {
                            data_left[10 + i] = plate[i];
                        }
                        send_data(data_left, sizeof(data_left));
                    }
                }
                else if(left == 0 && right == 1){
                    if(strlen(plate) == 0){
                        send_data(data_stop, sizeof(data_stop));
                    }
                    else {
                        uint8_t i,j;
                        for (i = 6-strlen(plate), j = 0; i < 6; i++, j++) {
                            data_right[6+i] = plate[j];
                        }
                        send_data(data_right, sizeof(data_right));
                    }
                }
                else{
                    ESP_LOGI("COMM","ERRO NA ORIENTACAO");
                }
            }
            else{
                ESP_LOGI("COMM","ERRO NO INDENTIFICADOR DO PAINEL");
            }

            status = false;
        }
        vTaskDelay(100);
    }
}

void store_communication_id(uint8_t id) {
    ESP_LOGE("store", "id: %d", id);

    nvs_handle_t communication_nvs_handle;

    esp_err_t err = nvs_open(name_space, NVS_READWRITE, &communication_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening communication_namespace: %s", esp_err_to_name(err));
        return;
    }

    err = nvs_set_u8(communication_nvs_handle, "id_nvs", id);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error to store id: %s", esp_err_to_name(err));
    }

    // Efetivamente escrever as alterações no armazenamento permanente
    err = nvs_commit(communication_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS changes: %s", esp_err_to_name(err));
    }

    nvs_close(communication_nvs_handle);
}

void retrieve_communication_id(void) {
    nvs_handle_t communication_nvs_handle;

    esp_err_t err = nvs_open(name_space, NVS_READONLY, &communication_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error to open communication_namespace to retrieve");
        return;
    }

    // Recuperar a variável id_nvs
    err = nvs_get_u8(communication_nvs_handle, "id_nvs", &id_nvs);
    if (err != ESP_OK) {
        id_nvs = id_default;
        ESP_LOGI(TAG, "Error to retrieve id_nvs");
    }

    ESP_LOGE("retrieve", "id_nvs: %d", id_nvs);

    // Efetivamente escrever as alterações no armazenamento permanente
    err = nvs_commit(communication_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS changes: %s", esp_err_to_name(err));
    }
    
    nvs_close(communication_nvs_handle);
}

uint8_t return_id(){
    return id_display;
}

void commUpdateRedeTask(void *pvParameter) {
    while (1) {
        if(flag_erase){
            flag_erase = 0;
            nvs_flash_erase();
            esp_restart();
        }
        if(status_id){
            status_id = false;  
            store_communication_id(define_id);
            initialize_comunication();
            return;
        }
        if(status_rede){
            status_rede = false;
            if(id==id_display){
                change_rede(ip_c, gateway_c, netmask_c, dns_c);
                return;
            }
        }
        taskYIELD();
    }
}

void commPublishStatusTask(void *pvParameter){
    while (1){
        publish_status_rede(id_display, retrieve_ethernet_one_variable(IP), retrieve_ethernet_one_variable(GATEWAY), retrieve_ethernet_one_variable(NETMASK), retrieve_ethernet_one_variable(DNS));
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

void set_message_ip(bool flag){
    uint8_t data_auto_ip[] = {0x00, 0x96, 0x03, 0xFF, 0xC5, 0xC5, 0x01, 'A', 'U', 'T', 'O', ' ', 'I', 'P', 0x11};
    uint8_t data_static_ip[] = {0x00, 0x96, 0x03, 0xFF, 0xC5, 0xC5, 0x01, 'S', 'T', 'A', 'T', 'I','C', ' ', 'I', 'P', 0x11};
    uint8_t data_static[] = {0x00, 0x96, 0x03, 0xFF, 0xC5, 0xC5, 0x01, 'S', 'T', 'A', 'T', 'I','C', 0x11};
    if(id_display == 1){
        if(flag){
            send_data(data_auto_ip, sizeof(data_auto_ip));
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        else{
            send_data(data_static_ip, sizeof(data_static_ip));
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
    else{
        if(flag){
            send_data(data_auto_ip, sizeof(data_auto_ip));
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        else{
            send_data(data_static, sizeof(data_static));
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}

void set_message_offline(){
    uint8_t data_offline[] = {0x00, 0x96, 0x03, 0xFF, 0xC5, 0xC5, 0x01, 'O', 'F', 'F', '-', 'L','I', 'N', 'E', 0x11};
    send_data(data_offline, sizeof(data_offline));
}

void status_ip(bool flag_ip){
    uint8_t data_stop[] = {0x00, 0x96, 0x03, 0xFF, 0XC5, 0XC5, 0x1F, 0x62, 0x11};
    uint8_t data_ip_ok[] = {0x00, 0x96, 0x04, 0xFF, 0xC5, 0xC5, 0x01, 'I', 'P', ' ', 'O', 'K',0x11};
    if(flag_ip){
        send_data(data_ip_ok, sizeof(data_ip_ok));
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    vTaskDelay(pdMS_TO_TICKS(3000));
    send_data(data_stop, sizeof(data_stop));
}

void initialize_comunication(){
    id_nvs = id_default;
    retrieve_communication_id();
    id_display = id_nvs;
}