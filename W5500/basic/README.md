🔍 Overview
Esse examplo demonstra um uso basico de `Ethernet driver` junto com `tcpip_adapter`. Os funcionamento global segue:
1. Instalação do driver Ethernet.
2. Envio de requerimento DHCP e aguardo da concessão
3. Aguardo da obtenção de endereço IP.
4. Inicializa cliente MQTT/MQTTS.
5. Após o cliente MQTT/MQTTS ser inicializado, é possível se inscrever e publicar em tópicos.

💻 Pré-Requisitos
Para esse projeto, é necessário ter:
-ESP32.
-Módulo Ethernet W5500. O módulo pode necessitar de uma fonte externa de alimentação. 3,3V, corrente superior a 200mA. 
-Realizar as conexões entre o ESP32 e o módulo. Essas conexões serão detralhas mais à frente.
-Cabo USB para programação
-ESP-IDF instalado na máquina.
-Cabo de rede Rj45.
-Broker MQTT/MQTTS em execução. Em caso de Broker MQTTS, é necessário ter o arquivo do certificado do servidor(CA).


🖱️Rodando o example
Foram criados 3 componentes, que estão localizados na pasta 'extra_commponents':

->ethe
Esse componente diz respeito à criação da conexão ethernet, garantindo a conexão de rede. Os valores inicias dos pinos são:
SCLK - 18
MOSI - 23
MISO - 19
CS0  -  5
INT0 -  4
Esses valores devem ser setados através do menuconfig da aplicação. Para isso, entre com 'idf.py menuconfig' no terminal do ESP-IDF. Na aba interativa que aparecer, vá em Example Configuration. Marque a opção 'SPI ETHERNET' e nas opções abaixo, informe que o Módulo a ser usado será o W5500. No exemplo, foi usado 'SPI clock speed (MHz)=12' e 'PHY Reset GPIO=-1'. Após isso, as configurações iniciais para uso do módulo terminaram.
A TAG do LOGI utilizada é "eth_example". Um manipulador de eventos está setado para verificar a conexão: caso a conexão seja perdida, ela será reestabelecida manualmente quando possível.
OBSERVAÇÃO IMPORTANTE:
Alguns handles que estão sendo chamados dentro de eth_main.c devem ser invocados somente 1 vez em cada execução da aplicação. É necessário um estudo de quais handles já estão sendo chamados dentro do código em questão para que não haja dupla chamada. Caso isso acontença, o ESP pode apresentar alguns erros na sua execução, como por exemplo, ficar reiniciando.
Para o caso de definição de IP estático, deve-se setar os valores das variáveis constantes estáticas ip, gateway e netmask (de acordo com a rede a ser usada). Caso o IP usado seja o automático, deve-ser comentar as linhas que antecedem a chamada `esp_eth_start(eth_handle_spi)`.

->mqtts
O componente mqtts pode ser usado tanto para estabelecer conexão com e sem incriptação. À partir do protocolo dessejado, setar o broker definido na variável EXAMPLE_BROKER_URI. No caso de uso de mqtts, o certificado (geralmente chamado de arquivo ca.crt) deve ser inserido na pasta ´certificats´. Nos casos testados, é necessário converter o arquivo para que o mesmo tenha formado .pem. Caso seja esse o caso, rode o comando `openssl x509 -in {mycert.crt} -out broker_ca.pem -outform PEM`, mudando os nomes para o seu arquivo. O nome do arquivo resultante deve ser broker_ca.pem. No exemplo dessa aplicação, há um subscribe automático quando a conexão é estabelecida. Caso não seja necessário, remover a linha de subscribe do gerenciador de eventos `mqq_event_handler`. Também nesse caso de teste, há uma mensagem sendo publicada à cada 1 segundo, sendo chamada no main.c do projeto. O tópico de publish/subscribe é setado em `MQTT_TOPIC`, que deve ser alterado para o seu broker utilizado.
Caso o uso seja sem incriptação, deve-ser remover a linha que define o .pem no mqtt_cfg. Para isso, comentar ou remover a linha `.cert_pem =  (const char *)broker_cert_pem_start`. Caso o broker em questão tenha username e password, deve-se setar seus valores nas variáveis `.username` e `.password`. Caso contrário, pode deixar como uma string em branco ou removê-las.

->ping
Esse componente foi criado para testar a efetiva conexão internet a partir do módulo ethernet utilizado. Como exemplo, dentro do código main.c, está setado o endereço do Google para teste do ping. Serão realizados 4 envio de pacotes por padrão. A função initialize_ping() que cria o serviço de ping não esta sendo chamada no main.c. 