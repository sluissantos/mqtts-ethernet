### 🔍 Overview
Esse examplo demonstra um uso basico de `Ethernet driver` junto com `tcpip_adapter`. Os funcionamento global segue:
1. Instalação do driver Ethernet.
2. Envio de requerimento DHCP e aguardo da concessão.
3. Aguardo da obtenção de endereço IP.
4. Inicializa cliente MQTT/MQTTS.
5. Após o cliente MQTT/MQTTS ser inicializado, é possível se inscrever e publicar em tópicos.
6. Iniciar o controlador bluetooth low energy (BLE).
7. Iniciar um serviço Nordick.

### 💻 Pré-Requisitos
Para esse projeto, é necessário ter:
-ESP32.
-Módulo Ethernet W5500. O módulo pode necessitar de uma fonte externa de alimentação. 3,3V, corrente superior a 200mA. 
-Realizar as conexões entre o ESP32 e o módulo. Essas conexões serão detralhas mais à frente.
-Cabo USB para programação
-ESP-IDF instalado na máquina.
-Cabo de rede Rj45.
-Broker MQTT/MQTTS em execução. Em caso de Broker MQTTS, é necessário ter o arquivo do certificado do servidor(CA).
-Level Shifter para converter, em fase, o sinal 3.3V para 5V (TX). Foram confeccionados manualmente usando dois transistores, um resistor de 1K e dois resistores de 10K.

<img src="resource/level_shifter.png" alt="Level Shifter">


### 🖱️ Componentes
Foram criados alguns componentes, que estão localizados na pasta 'extra_commponents':

`ethe`
Esse componente diz respeito à criação da conexão ethernet, garantindo a conexão de rede. Os valores inicias dos pinos são:
```
SCLK - 18
MOSI - 23
MISO - 19
CS0  -  5
INT0 -  4
GND  - GND
5V   - 5V
```
**IMPORTANTE: A ALIMENTAÇÃO DE TODOS OS COMPONENTES DERIVA DO DISPLAY. ENTÃO OS PINOS DE ALIMENTAÇÃO 5V DO ESP32 E DO W5500 DEVEM ESTAR CONECTADOS AO 5V DO DISPLAY**
```
5V      - VERMELHO
GND     - PRETO
RX UART - VERDE
```
<img src="resource/conector.png" alt="Conector.">

<img src="resource/exemplo.png" alt="Protótipo montado.">


Esses valores devem ser setados através do menuconfig da aplicação. Para isso, entre com 'idf.py menuconfig' no terminal do ESP-IDF. Na aba interativa que aparecer, vá em Example Configuration. Marque a opção 'SPI ETHERNET' e, nas opções abaixo, informe que o Módulo a ser usado será o W5500. No exemplo, foi usado 'SPI clock speed (MHz)=12' e 'PHY Reset GPIO=-1'. Após isso, as configurações iniciais para uso do módulo terminaram.
A TAG do LOGI utilizada é "eth_example". Um manipulador de eventos está setado para verificar a conexão: caso a conexão seja perdida, ela será reestabelecida automaticamente quando possível.
OBSERVAÇÃO IMPORTANTE:
Alguns handles que estão sendo chamados dentro de eth_main.c devem ser invocados somente 1 vez em cada execução da aplicação. É necessário um estudo de quais handles já estão sendo chamados dentro do código em questão para que não haja dupla chamada. Caso isso acontença, o ESP pode apresentar alguns erros na sua execução, como por exemplo, ficar reiniciando.
Para o caso de definição de IP estático, deve-se setar os valores das variáveis constantes estáticas ip, gateway e netmask (de acordo com a rede a ser usada). Caso o IP usado seja o automático, deve-ser comentar as linhas que antecedem a chamada `esp_eth_start(eth_handle_spi)`.

`mqtts`
O componente mqtts pode ser usado tanto para estabelecer conexão com e sem incriptação. À partir do protocolo dessejado, setar o broker definido na variável EXAMPLE_BROKER_URI. No caso de uso de mqtts, o certificado (geralmente chamado de arquivo ca.crt) deve ser inserido na pasta ´certificats´. Nos casos testados, é necessário converter o arquivo para que o mesmo tenha formado .pem. Caso seja esse o caso, rode o comando `openssl x509 -in {mycert.crt} -out broker_ca.pem -outform PEM`, mudando os nomes para o seu arquivo. O nome do arquivo resultante deve ser broker_ca.pem. No exemplo dessa aplicação, há um subscribe automático quando a conexão é estabelecida. Caso não seja necessário, remover a linha de subscribe do gerenciador de eventos `mqq_event_handler`. Também nesse caso de teste, há uma mensagem sendo publicada à cada 1 segundo, sendo chamada no main.c do projeto. O tópico de publish/subscribe é setado em `MQTT_TOPIC`, que deve ser alterado para o seu broker utilizado.
Caso o uso seja sem incriptação, deve-ser remover a linha que define o .pem no mqtt_cfg. Para isso, comentar ou remover a linha `.cert_pem =  (const char *)broker_cert_pem_start`. Caso o broker em questão tenha username e password, deve-se setar seus valores nas variáveis `.username` e `.password`. Caso contrário, pode deixar como uma string em branco ou removê-las.
Esse componente tem como parâmetros prê-setados 3 topicos (char *), sendo um deles para o publish (`arcelor/status`) da mensagem de Status que tem como intuito informar que o sistema está em funcionamento e mantendo a conexão com o broker, outro tópico (`arcelor/rede`) para o envio das configurações de rede que o esp32 irá realizar e o último tópico (`arcelor/message`) para o envio dos dados que devem ser tratados e posteriormente enviados ao display.

`uart`
O componente uart realiza as configurações iniais das portas de envio/recebimento UART, usada para fazer a comunicação com o display. Nesse exemplo, foi setado que a interface UART usada será a UART2, configurando os pinos:
```
UART2_PIN_RX  16
UART2_PIN_TX  17
```
Caso haja necessidade de alterar tais pinos, basta redefini-los nos defines presentes em uart.h. No nosso caso, como é necessário somente envio de informações para o display, nos preocuparemos somente com o pino TX.

`communication`
O componente communication é uma interface de comunicação usada para estabelecer o envio de dados para o display. Esse componente possui diversas funções usadas tanto pelo componente bleuartServer quanto pelo componente mqtts_eth. As mensagens recebida pelo tópico `arecelor/message` será enviada para esse componente e será tratada, gerando as saídas pré-estabelecidas. 

`blemananger` e `bleuartServer`
Esse componentes garantem a possibilidade de conexão bluetooth com o esp32, através de um smarthphone, principalmente. O nome do dispositivo será uma concatenação entre a string setada dentro da função void `bleuartServerInit(uint16_t ID)` e por esse parâmetro passado na função, quando iniciado o servidor.

### Mensagens setadas no display
Já estão definidas algumas mensagens padrão para essa aplicação a partir do estado do esp32 e das mensagens recebidas:

**AUTO IP**
Essa mensagem aparece quando a variável não volátil `ip` armazenada na nvs_flash é vazia. Uma vez que essa variável não está vazia, o algoritmo entende que é para ser estabelecida uma conexão ethernet com ip automático.

**IP OK**
Aparece somente para quando é ip automático. Uma vez que a conexão é estabelecida com sucesso e o ip automático é obtido, essa mensagem irá aparecer momentaneamente no display.

**STATIC IP**
Já essa mensagem irá aparecer quando for setado um ip específico (variável `ip` não vazia). Para esse caso, não é possível obter um status da conexão de rede a partir dos dados enviados. Para validar a conexão, basta verificar o tópico de status no broker ou fazer o envio de alguma mensagem válida.

**🔒**
Essa mensagem irá aparecer sempre que não for setada nenhuma mensagem de texto no display ou depois de que uma nova conexão for estabelecida.

### Definindo ip a partir de uma mensagem MQTT
Após uma conexão de rede ser estabelecida, automaticamente será subscrevido o tópico `arcelor/rede`. O padrão de mensagem a ser enviado é:

```
{
  "ip":"192.168.15.100",
  "gateway":"192.168.15.1",
  "netmask":"255.255.255.0",
  "dns":"8.8.8.8"
}
```
Através dos valores definidos para esses objetos, será definido os valores da rede (estática) e que serão gravados na memória flash, sendo esse o default após qualquer reinicialização.
É possível setar cada parâmetro individualmente, bastando somente enviar o objeto desejado. 
Exemplo:
```
{
  "ip":"192.168.15.100"
}
```

**Objeto "erase"**
Para limpar os valores da memória e definir um ip automático, mandar um json com o objeto ```"erase":1```. Caso esse objeto seja diferente de 0, o restantes da mensagem é ignorada. Esse caso so é possível casa haja conexão internet. Exemplo:
```
{
  "erase":0
}
```

## IMPORNTANTE
**TANTO AS CONFIGURAÇÕES DA REDE QUANTO A DEFINIÇÃO DO ID DO PAINEL QUE POR VENTURA TENHA SIDO MUDADA IRÃO SER APAGADAS.**
**ATÉ O MOMENTO, É SUGERÍVEL ESTABELECER QUAL TIPO DE CONEXÇÃO (AUTOMÁTICA OU ESTÁTICA) PRIMEIRAMENTE E DEPOIS, REDEFINIR O ID DO PAINEL, CASO NECESSÁRIO**

### Definir ip a partir de uma mensagem bluetooth
Após estabelecer uma conexão bluetooth com o dispositivo, haverá a possibilidade de setar os parâmetros da rede por mensagem pré-definidas.
É possível definir cada parâmetro da rede individualmente ou através de uma única mensagem. A mensagem deve ser enviada em formato de hexadecimal. Também há a possibilidade de realizar um erase nos dados preenchidos na memória flash através de um opcode.
Há os seguintes opcodes definidos:

**opcode 00**:
Usando esse opcode, é possível definir todos os parâmetros da rede através de uma única mensagem. 
Exemplo:```00C0A80F64C0A80F01FFFFFF0008080808```: após o primeiro hexadecimal informar o valor do opcode, deve inserir imediatamente após ele os valores de ip, gateway, netmask e dns. No caso de exemplo temos que:
`
ip = 00**C0A80F64**C0A80F01FFFFFF0008080808 = 192.168.15.100;
gateway = 00C0A80F64**C0A80F01**FFFFFF0008080808 = 192.168.15.1;
netmask = 00C0A80F64C0A80F01**FFFFFF00**08080808 = 255.255.255.0; 
dns = 00C0A80F64C0A80F01FFFFFF00**08080808** = 8.8.8.8.
` 

**opcode 02**:
Define somente o ip da rede.
Exemplo: ```02C0A80F64``` = 192.168.15.100.

**opcade 03**:
Define o gateway.
Exemplo: ```03C0A80F64``` = 192.168.15.1.

**opcode 04**
Define a máscara de rede.
Exemplo: ```04FFFFFF00``` = 255.255.255.0.

**opcade 05**
Define o DNS da rede. Usado como o padrão o DNS público do Google, que permite reconhecer o domínio do ambiente QA que utilizamos.
Exemplo: ```08080808``` = 8.8.8.8.

**opcode 06**
Realiza um erase na memória flash, limpando todos os parâmetros de rede uma vez setados.
Exemplo: ```06``` = clean flash memory. Após esse comando, será tentada uma conexão de rede com ip automático.

**opcode 07**
Realiza a definição do id do display. Esse id será armazenado na memória interna do esp32 e será definido como padrão após qualquer reinicialização. Para limpar, realizar um erase.
Exemplo: ```0700``` = ao enviar esse comando em HEXA, é definido que o ip do display será 0 (definido através do segundo hexadecimal enviado).


### Definir o ID do display através de uma mensagem MQTT
Quando a aplicação é inicializada, ela automaticamente se subscreve no tópico `"arcelor/{MAC-WIFI}`.
Para definir qual o id do display, basta enviar uma mensagem mqtt para esse tópico informando qual será o id do display conectado ao esp32 que possui tal MAC.
Exemplo: **tópico: arcelor/A0:B7:65:61:78:C0**

```
{
  "define":1
}
```
Com essa mensagem, definimos que o id do display será 1 (display central). Lembrando que os id's disponíveis são:
id=0, display mais à esquerda;
id=1, display central (maior display que também exibirá a placa);
id=2, display mais à direita.

### Definir as mensagens exibidas no painel
Há duas formas de exibição padrão já setadas, uma que o intuito inicial é mostrar um placa de veículo e informar em qual direção seguir.
A mensagem de exibição será setada somente por MQTT. O padrão de mensagem é:
```
{
  "id":1,
  "left":0,
  "right":1,
  "plate":"LOG2023",
  "data":[]
}
```
**Objeto "id"**
Cada painel será gravado com um firmware que irá pré-definir qual tipo de painel é, ja que temos duas possibilidades de painel: um que mostra a placa e a direção e outro que mostra somente a direção. 
Todos os paineis irão se subscrever no mesmo tópico de mensagem (`arcelor/message`). 
Com "id":0, é mandada um mensagem para o display à esquerda. Com "id":1, é mandada uma mensagem para o display central. Com "id":2, quem irá receber a mensagem será o display à direita. Somente o display central irá utilizar os objetos "plate" e "data".

**Objeto "left" e "right"**
Define qual o sentido em que o veículo deve seguir. É importante que ambos sejam logicamente opostos: caso queira que o sinal seja para que o veículo vá para a esquerda, deve-se definir "left":1 e "right":0, assim, setas com sentido para a esquerda se deslocarão para a esquerda no painel. Para orientar que o veículo siga para a direita, deve definir "left":0 e "right":1, assim, setas com sentido para a direita se deslocarão para a direita no painel. Os símbolos e dinâmicas da mensagem padrão já são definidos. Para conhecê-las, vide prática.

**Objeto "plate"**
Esse objeto define qual será a placa a ser exibida no painel. É importante que o objeto contenha uma string de 7 posições preenchidas.
Caso esse objeto contenha uma string vazia ("plate":""), irá definir que um cadeado 🔒 seja plotado no display, ignorando os objetos "left" e "right".

**Objeto "data"**
Define diretamente a mensagem que será enviada para o display. É necessário que seja uma mensagem válida conforme especificação do próprio painel. Para isso, procure o manual de utilização do mesmo, que se encontra no nesse diretório em `resource`.
Esse objeto deve ser preenchido com valores decimais conforme sua necessidade. É importante ressaltar que, uma vez que esse objeto esteja diferente de vazio, será ignorado os restantes dos objetos presentes na mensagem, caso seja uma mensagem válida. Exemplo:
```
{
  "id":1,
  "left":0,
  "right":1,
  "plate":"LOG2023",
  "data":[0, 150, 3, 255, 197, 197, 31, 0, 17]
}
```
Nesse exemplo, é definido que seja plotado no display o desenho de um cadeado ☎️ no display 1 (display central).

É imporante ressaltar que os envios da mensagem para o painel contém um delay, devido a limitações do próprio display. Logo, quanto maior a mensagem, maior será o tempo necessário de envio e de efetiva plotagem no display.