/**
 *  Propriétario: LogPyx
 *  
 *  Nome: device.c
 *  
 *  Descrição: Trata funções relacionadas aos devices (periféricos), gerenciamento de conexão,
 *  configuração, infomarções sobre.  
 * 
 *  Referências:
 *
 *  Desenvolvedor: Henrique Ferreira 
 */

////////////////////////////////////////////////////////////////////////////////////
//Includes Aura
////////////////////////////////////////////////////////////////////////////////////

#include "device.h"
//#include "storage.h"
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////
//Private Defines
////////////////////////////////////////////////////////////////////////////////////
#define DEVICE_BLE1_INDX 0                       // Indice para gerenciar dispositivo conectado BLE
#define DEVICE_BLE2_INDX 1                       // Indice para gerenciar dispositivo conectado BLE
#define DEVICE_BLE3_INDX 2                       // Indice para gerenciar dispositivo conectado BLE
#define DEVICE_BLE4_INDX 3
#define DEVICE_UART_INDX 4                       // Indice para gerenciar dispositivo conectado a uart
#define DEVICE_MIN_DISTANCE_MEASUREMENT 500      //mm
#define DEVICE_MAX_DISTANCE_MEASUREMENT 20000    // 

#define DEVICE_TASK_DELAY            500// 200      // ms
////////////////////////////////////////////////////////////////////////////////////
//Global variables
////////////////////////////////////////////////////////////////////////////////////
// Lista de dispositivos
device_t deviceList[DEVICE_QNT] ={DEVICE_DEFAULT(),
                                  DEVICE_DEFAULT(),
                                  DEVICE_DEFAULT(),
                                  DEVICE_DEFAULT()};

uint8_t inConnectionProcess =0;

int8_t indxCelular = -1;

uint8_t requestPeriod = 1;
////////////////////////////////////////////////////////////////////////////////////
//Private Declaration
////////////////////////////////////////////////////////////////////////////////////
uint16_t deviceSetSecurityNear(uint8_t deviceIndx, uint16_t distance);
uint16_t deviceSetSecurityFar(uint8_t deviceIndx, uint16_t distance);
void deviceRotinaLed();
////////////////////////////////////////////////////////////////////////////////////
//Implementations
////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
// Função: deviceCheckConnectionTask();
// Descrição: Rotina para verificar se existe algum device desconectado. 
// Se algum dispositvo encontra se desconectado, é realizado uma tentativa de conexão.
// Argumentos: void
// Retorno:    void 
//////////////////////////////////////////////////////////////////////////////////
void deviceConnectionTask(void * pvParameter){
     static uint8_t deviceIndx=0;
      uint8_t count=19; 
    //  uint16_t dist[]={100,200,300,400, 500, 600, 700, 800, 900,1000,1100,1200,1300,1400,1500,1600,1700,1800,1900,2000,
     // 1900,1800,1700,1600,1500,1400,1300,1200,1100,1000,900,800,700,600,500,400,300,200,100};
     const TickType_t xDelay = DEVICE_TASK_DELAY / portTICK_PERIOD_MS;
    while(1){
      
         if(deviceList[deviceIndx].status == DEVICE_STATUS_WAIT_FOR_CONECTING){
                // Testa se o dispositivo esta conectado
               deviceConnect(deviceIndx);
                
        }else{
            if((deviceList[deviceIndx].status == DEVICE_STATUS_CONNECTING) &
               (deviceList[deviceIndx].timeout >= DEVICE_TIMEOUT_LIMITE) ){
                    deviceList[deviceIndx].status = DEVICE_STATUS_WAIT_FOR_CONECTING;
               }
        }
        deviceIndx++;
        //deviceIndx = deviceIndx%(DEVICE_QNT-1);
       // ESP_LOGI("DeviceTask","DeviceIndx: %d",deviceIndx);
        
        
       // dataManagerPickPlaceEngine(dist[count]);
        count++;
        count=count%39;

        //deviceRotinaLed();
        vTaskDelay(xDelay);
    }
}

   
///////////////////////////////////////////////////////////////////////////////////
// Função: deviceConnect(uint8_t deviceIndx);
// Descrição: Conecta um dispositivo caso exista um link disponível no BLE serve.
// Argumentos: uint8_t deviceIndx -> DEVICE_UART_INDX
//                                   DEVICE_BLE1_INDX
//                                   DEVICE_BLE2_INDX
//                                   DEVICE_BLE3_INDX
// Retorno:    int8_t gattsConnectionID -> 0 a 2 conexões válidas
//                                        -1     sem conexões 
//////////////////////////////////////////////////////////////////////////////////
#define TEMPO 61
int8_t deviceConnect(uint8_t deviceIndx){
    static uint8_t toogle=0;
    static uint16_t localTimer=0;
    int8_t connAvailable =-1;
     uint8_t data[1] = {deviceIndx};
     // ESP_LOGI("DeviceConn","DeviceIndx: %d GATTS_CONN: %d Interface: %d",deviceIndx,deviceList[deviceIndx].gattsConnectionID,deviceList[deviceIndx].interface);
   switch (deviceList[deviceIndx].interface){
        
        case COMM_INTERFACE_UART:
            //commSendCommand(deviceList[deviceIndx],RD_REQUEST_CONNECTION,data,2);
        break;

        case COMM_INTERFACE_BLE_SERVER:
           //ESP_LOGI("DeviceTask","Device: %d GATTS_CONN %d",deviceIndx,deviceList[deviceIndx].gattsConnectionID);
            // Se dispositivo não esta conectado 
           /* if(deviceList[deviceIndx].isConnected == 0){
                // Verifica se o dispositivo possui um id de conexão valido
                if(deviceList[deviceIndx].gattsConnectionID >-1)
                {    
                     // Solicita conexao (Caso ocorra sucesso ao registrar o device porém falha na recepção de dados)
                     // Device so é considerado ativo após receber uma reposta valida devido ao comando  
                      //  commSendCommand(deviceList[deviceIndx],RD_REQUEST_CONNECTION,data,2);
                      
                        barcodeData_t barcodeData;

                        //localTimer = dataManagerIncrementTimer();

                        barcodeData.devAddr = 0x2347;
                        barcodeData.tmst = (clock() * 1000 / CLOCKS_PER_SEC);
                        
                        if(toogle){ // LAT01
                        barcodeData.event_type=toogle;
                        barcodeData.position.pick.anchor_id=0x4387;
                        barcodeData.position.pick.tmst= (clock() * 1000 / CLOCKS_PER_SEC);;
                        barcodeData.position.pick.height = 0;
                        barcodeData.position.pick.x = -17.88*1000;
                        barcodeData.position.pick.y = 55.88*1000;
                           toogle = 0;
                         
                        }else{
                         // JC05
                        barcodeData.event_type=toogle;
                        barcodeData.position.place.anchor_id=0x4387;
                        barcodeData.position.place.tmst= (clock() * 1000 / CLOCKS_PER_SEC);;
                        barcodeData.position.place.height = 0;
                        barcodeData.position.place.x = 47.65*1000;
                        barcodeData.position.place.y = 63.23*1000;
                        toogle = 1;
                        }
                        
                       
                        barcodeData.codes[0].ch = 0;
                        strcpy(barcodeData.codes[0].type,"barcod");
                        strcpy(barcodeData.codes[0].subType,"128code");
                        strcpy(barcodeData.codes[0].code,"AB12345678910");

                        barcodeData.codes[1].ch = 1;
                        strcpy(barcodeData.codes[1].type,"barcod");
                        strcpy(barcodeData.codes[1].subType,"128code");
                        strcpy(barcodeData.codes[1].code,"A23345678910");
                        
                        dataManagerFakePickAndPlace(barcodeData,deviceList[0]);
                       // jsonBarcode(barcodeData,deviceList[0]);

                      // ESP_LOGI("COMM_TAG ","teste");

                     // deviceList[deviceIndx].status = DEVICE_STATUS_CONNECTING;
                      deviceList[deviceIndx].timeout =0;
                      if(deviceList[deviceIndx].timeout >=15){//7
                        // Caso a conexão se mantenha estavel e sem resposta
                        // Este deivce é classificado como Celular
                        deviceList[deviceIndx].isConnected = 1;
                        indxCelular = deviceIndx;
                      }
                     // ESP_LOGI("DEVICE_TAG","RD_REQUEST_CONNECTION  %d GATTS_CONN %d",deviceIndx,deviceList[deviceIndx].gattsConnectionID);
                }
                else
                {
                    // Registra o dispositivo com um id de conexão valido
                    if(gattuartRegisterDevice(deviceIndx) > -1){
                
                        // Envia comando solicitando conexão (dados do device)
                        commSendCommand(deviceList[deviceIndx],RD_REQUEST_CONNECTION,data,2);
                       // deviceList[deviceIndx].status = DEVICE_STATUS_WAIT_FOR_CONECTING;
                       // deviceList[deviceIndx].timeout++;
                        ESP_LOGI("DEVICE_TAG","RD_REQUEST_CONNECTION  %d GATTS_CONN %d",deviceIndx,deviceList[deviceIndx].gattsConnectionID);
                    }                    
                }               
            }*/
           
              
            /*  if( bleuartClientGetConnDeviceC() || bleuartClientGetConnDeviceB() || bleuartClientGetConnDeviceA()){
                  alarmSetLed();
                }else{
                   alarmAcionaLed();
                } */
              
           
    /*       if(deviceList[deviceIndx].isConnected){
             
              barcodeData_t barcodeData;

              barcodeData.devAddr = 0x2347;
              barcodeData.tmst = (clock() * 1000 / CLOCKS_PER_SEC);
                       
              if(toogle){ // LAT01
                barcodeData.event_type=toogle;
                barcodeData.position.pick.anchor_id=0x4387;
                barcodeData.position.pick.tmst= (clock() * 1000 / CLOCKS_PER_SEC);
                barcodeData.position.pick.height = 0;
                barcodeData.position.pick.x = -17.88*1000;
                barcodeData.position.pick.y = 55.88*1000;
                toogle = 0;
                         
                }else{
                // JC05
               barcodeData.event_type=toogle;
               barcodeData.position.place.anchor_id=0x4387;
               barcodeData.position.place.tmst= (clock() * 1000 / CLOCKS_PER_SEC);;
               barcodeData.position.place.height = 0;
               barcodeData.position.place.x = 47.65*1000;
               barcodeData.position.place.y = 63.23*1000;
               toogle = 1;
               }
                        
                       
               barcodeData.codes[0].ch = 0;
               strcpy(barcodeData.codes[0].type,"barcod");
               strcpy(barcodeData.codes[0].subType,"128code");
               strcpy(barcodeData.codes[0].code,"AB12345678910");

               barcodeData.codes[1].ch = 1;
               strcpy(barcodeData.codes[1].type,"barcod");
               strcpy(barcodeData.codes[1].subType,"128code");
               strcpy(barcodeData.codes[1].code,"A23345678910");
                        
            //   dataManagerFakePickAndPlace(barcodeData,deviceList);
               
           }*/
        break;
                   
   }
      //deviceRotinaLed();
    /*  bleuartClientRequestTemp(0);

      localTimer++;
      localTimer = localTimer%TEMPO;

      if(localTimer == 0 ){
        bleuartClientOnOffCamera(0,1);
      }

      if(localTimer == 20){
         bleuartClientOnOffCamera(1,1);
      }*/
      

    return deviceList[deviceIndx].gattsConnectionID;
}

///////////////////////////////////////////////////////////////////////////////////
// Função: deviceDisconnect(uint8_t deviceIndx);
// Descrição: Função responsável por disconectar dispositivo
// Argumentos: uint8_t deviceIndx -> DEVICE_BLE1_INDX
//                                   DEVICE_BLE2_INDX
//                                   DEVICE_BLE3_INDX
// Retorno:    int8_t deviceIndx ->  
// Obs: Função tem apenas efeito a dispositivos conectados via bluetooth
//////////////////////////////////////////////////////////////////////////////////
int8_t deviceDisconnect(uint8_t deviceIndx){
     uint8_t connId[1]  ={deviceIndx};
     //commSendCommand(deviceList[deviceIndx],RD_REQUEST_DISCONNECTION,&connId[0],2);
     ESP_LOGI("DEVICE_BLE","DISCONECTD: %d",deviceIndx);
     return deviceIndx;
}

///////////////////////////////////////////////////////////////////////////////////
// Função: deviceGet(uint8_t deviceIndx);
// Descrição: Retorna uma struct contendo os dados sobre disposito. 
// Argumentos: uint8_t deviceIndx -> DEVICE_UART_INDX
//                                   DEVICE_BLE1_INDX
//                                   DEVICE_BLE2_INDX
//                                   DEVICE_BLE3_INDX
//             
// Retorno:    device_t -> Struct contendo dados do device
//////////////////////////////////////////////////////////////////////////////////
device_t * deviceGet(uint8_t deviceIndx){
  return &deviceList[deviceIndx];
}

///////////////////////////////////////////////////////////////////////////////////
// Função: deviceGetCelular();
// Descrição: Retorna o device que carrega conexão do celular 
// Argumentos:              
// Retorno:    device_t -> Struct contendo dados do device
//////////////////////////////////////////////////////////////////////////////////
device_t * deviceGetCelular(){
  return &deviceList[indxCelular];
}

///////////////////////////////////////////////////////////////////////////////////
// Função: deviceSetIndxCelular();
// Descrição: Retorna o device que carrega conexão do celular 
// Argumentos:              
// Retorno:    device_t -> Struct contendo dados do device
//////////////////////////////////////////////////////////////////////////////////
void deviceSetIndxCelular(int8_t index){
    indxCelular = index;
}

///////////////////////////////////////////////////////////////////////////////////
// Função: deviceGetIndxCelular();
// Descrição: Retorna o indeice device que carrega conexão do celular 
// Argumentos:              
// Retorno:    uint8_t -> Struct contendo dados do device
//////////////////////////////////////////////////////////////////////////////////
uint8_t deviceGetIndxCelular(){
   return indxCelular;
}
