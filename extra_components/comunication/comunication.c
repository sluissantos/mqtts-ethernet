#include "comunication.h"

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

uint8_t id;
uint8_t left;
uint8_t right;
static char* plate;
static bool status=false;

// Estrtura buffer circular
typedef struct buffer{   
    uint16_t maxlen;
    uint16_t size;
    uint16_t head;
    uint16_t tail;
    uint8_t buffer[COMM_BUFFER_SIZE];
}buffer_t;

// Inicializa buffer BLE
buffer_t uartBuffer ={
    .maxlen = COMM_BUFFER_SIZE,
    .size = 0,
    .head = 0,
    .tail = 0,
    .buffer={0},
};

void parse_json(const char* jsonString) {
    cJSON* json = cJSON_Parse(jsonString);
    if (json == NULL) {
        printf("Erro ao fazer o parse do JSON.\n");
        return;
    }
    
    cJSON* id_object = cJSON_GetObjectItem(json, "id");
    if (id_object != NULL) {
        id = id_object->valueint;
    } else {
        id = 2;
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
        plate = "";
    }

    // Após usar o JSON, lembre-se de liberar a memória ocupada por ele.
    cJSON_Delete(json);
	return;
}

void set_variables(char *payload){
	parse_json(payload);
	status =true;
	return;
}

void commMontaPacote(uint8_t *data);
int8_t BufferPop(buffer_t * buffer, uint8_t *data);
int8_t BufferPush(buffer_t * currentBuffer,uint8_t * newData);

int8_t commBufferPush(uint8_t interface,uint8_t * newData){
	int8_t ret=0;
	switch (interface){
		case COMM_INTERFACE_UART:
			break;
		default:
			break;
	}
	return ret;
}

///////////////////////////////////////////////////////////////////////////////////
// Função: commBufferPush(uint8_t * newData);
// Descrição: Realiza inserção de novos dados ao buffer
// Argumentos: buffer_t *buffer -> Ponteiro para buffer de dados
// 		       uint8_t   data 	-> Struct contendo dados do device
// Retorno:
//////////////////////////////////////////////////////////////////////////////////
int8_t BufferPush(buffer_t * currentBuffer,uint8_t * newData)
{	//buffer_t *buffer = &primaryBuffer;
	buffer_t *buffer = currentBuffer;
    int8_t next;	
	uint8_t data = *newData;		

    next = buffer->head + 1;  // next is where head will point to after this write.
    if (next >= buffer->maxlen)
        next = 0;

    if (next == buffer->tail)  // if the head + 1 == tail, circular buffer is full
        return -1;

    buffer->buffer[buffer->head] = data;  // Load data and then move
    buffer->head = next;             // head to next data offset.
    return 0; // return success to indicate successful push.
}

///////////////////////////////////////////////////////////////////////////////////
// Função: commBufferPop(buffer_t * buffer, uint8_t * data);
// Descrição: Envia dado para dispositivo selecionado
// Argumentos: buffer_t * buffer -> Ponteiro para buffer
// 			   uint8_t * data 	 -> Ponteiro para byte recolhido
// Retorno
//////////////////////////////////////////////////////////////////////////////////
int8_t BufferPop(buffer_t * buffer, uint8_t *data){
    int8_t next;

    if (buffer->head == buffer->tail)  // if the head == tail, we don't have any data
        return -1;

    next = buffer->tail + 1;  // next is where tail will point to after this read.
    if((next >= buffer->maxlen))
        next = 0;

    *data = buffer->buffer[buffer->tail];  // Read data and then move
    buffer->tail = next;              // tail to next offset.
    return 0;  // return success to indicate successful push.
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

///////////////////////////////////////////////////////////////////////////////////
// Função: commUpdateBufferTask(uint16_t );
// Descrição: Define nova de decawaveID 
// Argumentos: uint8_t deviceIndx -> DEVICE_UART_INDX
//             uint8_t length -> Tamanho do vetor data + 1 devido ao OPCODE
// Retorno:    device_t -> Struct contendo dados do device
//////////////////////////////////////////////////////////////////////////////////
void commUpdateBufferTask(void *pvParameter) {
   while (1) {
        if (status) {
            //printf("id=%d\n", id);
            //printf("left=%d\n", left);
            //printf("right=%d\n", right);
            //printf("plate=%s\n", plate);
            uint8_t dataSize = snprintf(NULL, 0, "%d%d%d%s", id, left, right, plate) + 1;
            uint8_t *data = (uint8_t *)malloc(dataSize);
            if (data != NULL) {
                snprintf((char *)data, dataSize, "%d%d%d%s", id, left, right, plate);
                printf("data=%s\n", (char *)data);
                printf("length=%hhu\n", dataSize);
                commMontaPacote(data);
                free(data);
            }
            status = false;
        }
        taskYIELD();
    }
}

///////////////////////////////////////////////////////////////////////////////////
// Função: commMontaPacote(uint8_t ByteLido);
// Descrição: Monta os dados recebidos em um pacote valido
// Argumentos: uint8_t deviceIndx -> DEVICE_UART_INDX             
// Retorno:    device_t -> Struct contendo dados do device
//////////////////////////////////////////////////////////////////////////////////
void commMontaPacote(uint8_t *data){
    int8_t ret=0;
    ESP_LOGI("COMM","data=%hhu", data[0]);
	//  Decodificacao do frame recebido
	//	------------------------------------------------------
	// Le cada byte do buffer circular da Uart, e monta os 
	// pacotes recebidos em um buffer local: new_pacote.
	//
	// gEstadosRxSerial assume os seguites valores:
	// 0 = Estado inicial
	// 1 = Recebeu start bit     
	// 2 = Recebeu tamanho do frame
	// 3 = Recebeu dado
	// 4 = Recebeu chksum, dado disponivel, new pacote 
	// 5 ao 255 = Reservado
	// Ao final do frame recebido, chama o metodo 
	// TrataPacote, que decodifica o frame
	//	------------------------------------------------------
	switch(data[0]) {
		case 48:
            ret=BufferPush(&uartBuffer, 0x00);
            ret=BufferPush(&uartBuffer, 0x96);
            break;

		case 49:	    			
            break;

		case 50:  		
			break;    			    			

		default:
            ESP_LOGI("COMM","DEFAULT");
			break;
	}
}

