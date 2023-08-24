#include "eth.h"

static char *TAG = "eth_example";
/*
static char *ip = "192.168.15.100";
static char *gateway = "192.168.15.1";
static char *netmask = "255.255.255.0";
static char *dns = "8.8.8.8";
*/

char *namespace = "eth_namespace";

char *ip = "";
char *gateway = "";
char *netmask = "";
char *dns = "";

bool flag_ip = true;

esp_eth_handle_t eth_handle_spi;
esp_netif_t *eth_netif_spi; 

static bool ip_address_obtained = false;

#define INIT_SPI_ETH_MODULE_CONFIG(eth_module_config, num)                                      \
    do {                                                                                        \
        eth_module_config.spi_cs_gpio = CONFIG_EXAMPLE_ETH_SPI_CS0_GPIO;           \
        eth_module_config.int_gpio = CONFIG_EXAMPLE_ETH_SPI_INT0_GPIO;             \
        eth_module_config.phy_reset_gpio = CONFIG_EXAMPLE_ETH_SPI_PHY_RST0_GPIO;   \
        eth_module_config.phy_addr = CONFIG_EXAMPLE_ETH_SPI_PHY_ADDR0;                \
    } while(0)

typedef struct {
    uint8_t spi_cs_gpio;
    uint8_t int_gpio;
    int8_t phy_reset_gpio;
    uint8_t phy_addr;
}spi_eth_module_config_t;

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data){
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data){
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ip_address_obtained = true;
}

void store_ethernet_ip(char *ip){
    ESP_LOGE("store", "ip: %s", ip);
    ESP_LOGE("store", "gateway: %s", gateway);
    ESP_LOGE("store", "netmask: %s", netmask);
    ESP_LOGE("store", "dns: %s", dns);

    nvs_handle_t ethernet_nvs_handle;

    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &ethernet_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening eth_namespace: %s", esp_err_to_name(err));
        return;
    }

    if(ip != NULL){
        err = nvs_set_str(ethernet_nvs_handle, "ip", ip);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error to stage ip: %s", esp_err_to_name(err));
        }
    }

    // Efetivamente escrever as alterações no armazenamento permanente
    err = nvs_commit(ethernet_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS ip changes: %s", esp_err_to_name(err));
    }

    nvs_close(ethernet_nvs_handle);

}

void store_ethernet_gateway(char *gateway){
    ESP_LOGE("store", "ip: %s", ip);
    ESP_LOGE("store", "gateway: %s", gateway);
    ESP_LOGE("store", "netmask: %s", netmask);
    ESP_LOGE("store", "dns: %s", dns);

    nvs_handle_t ethernet_nvs_handle;

    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &ethernet_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening eth_namespace: %s", esp_err_to_name(err));
        return;
    }

    if(gateway != NULL){
        err = nvs_set_str(ethernet_nvs_handle, "gateway", gateway);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error to stage gateway, %s", esp_err_to_name(err));
        }
    }

    // Efetivamente escrever as alterações no armazenamento permanente
    err = nvs_commit(ethernet_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS gateway changes: %s", esp_err_to_name(err));
    }

    nvs_close(ethernet_nvs_handle);
}

void store_ethernet_netmask(char *netmask){
    ESP_LOGE("store", "ip: %s", ip);
    ESP_LOGE("store", "gateway: %s", gateway);
    ESP_LOGE("store", "netmask: %s", netmask);
    ESP_LOGE("store", "dns: %s", dns);

    nvs_handle_t ethernet_nvs_handle;

    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &ethernet_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening eth_namespace: %s", esp_err_to_name(err));
        return;
    }

    if(netmask != NULL){
        err = nvs_set_str(ethernet_nvs_handle, "netmask", netmask);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error to stage netmask: %s", esp_err_to_name(err));
        }
    }

    // Efetivamente escrever as alterações no armazenamento permanente
    err = nvs_commit(ethernet_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS netmask changes: %s", esp_err_to_name(err));
    }

    nvs_close(ethernet_nvs_handle);
}

void store_ethernet_dns(char *dns){
    ESP_LOGE("store", "ip: %s", ip);
    ESP_LOGE("store", "gateway: %s", gateway);
    ESP_LOGE("store", "netmask: %s", netmask);
    ESP_LOGE("store", "dns: %s", dns);

    nvs_handle_t ethernet_nvs_handle;

    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &ethernet_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening eth_namespace: %s", esp_err_to_name(err));
        return;
    }

    if(dns != NULL){
        err = nvs_set_str(ethernet_nvs_handle, "dns", dns);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error to stage dns: %s", esp_err_to_name(err));
        }
    }

    // Efetivamente escrever as alterações no armazenamento permanente
    err = nvs_commit(ethernet_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS dns changes: %s", esp_err_to_name(err));
    }

    nvs_close(ethernet_nvs_handle);
}

void store_ethernet_variables(char *ip, char *gateway, char *netmask, char *dns){
    ESP_LOGE("store", "ip: %s", ip);
    ESP_LOGE("store", "gateway: %s", gateway);
    ESP_LOGE("store", "netmask: %s", netmask);
    ESP_LOGE("store", "dns: %s", dns);

    nvs_handle_t ethernet_nvs_handle;

    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &ethernet_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening eth_namespace: %s", esp_err_to_name(err));
        return;
    }

    if(ip != NULL){
        err = nvs_set_str(ethernet_nvs_handle, "ip", ip);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error to stage ip: %s", esp_err_to_name(err));
        }
    }

    if(gateway != NULL){
        err = nvs_set_str(ethernet_nvs_handle, "gateway", gateway);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error to stage gateway, %s", esp_err_to_name(err));
        }
    }

    if(netmask != NULL){
        err = nvs_set_str(ethernet_nvs_handle, "netmask", netmask);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error to stage netmask: %s", esp_err_to_name(err));
        }
    }

    if(dns != NULL){
        err = nvs_set_str(ethernet_nvs_handle, "dns", dns);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error to stage dns: %s", esp_err_to_name(err));
        }
    }

    // Efetivamente escrever as alterações no armazenamento permanente
    err = nvs_commit(ethernet_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS rede changes: %s", esp_err_to_name(err));
    }

    nvs_close(ethernet_nvs_handle);
}

void retrieve_ethernet_variable(void){
    nvs_handle_t ethernet_nvs_handle;

    esp_err_t err = nvs_open(namespace, NVS_READONLY, &ethernet_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error to open eth_namespace to retrieve");
        return;
    }

    char buffer[64]; // Buffer para armazenar as strings recuperadas

    // Recuperar as variáveis
    size_t required_size;
    err = nvs_get_str(ethernet_nvs_handle, "ip", NULL, &required_size);
    if (err == ESP_OK) {
        if (required_size <= sizeof(buffer)) {
            err = nvs_get_str(ethernet_nvs_handle, "ip", buffer, &required_size);
            if (err == ESP_OK) {
                //free(ip);
                ip = strdup(buffer); // Aloca memória e copia o valor
            }
        } else {
            ESP_LOGI(TAG, "Error to retrieve ip");
        }
    }

    err = nvs_get_str(ethernet_nvs_handle, "gateway", NULL, &required_size);
    if (err == ESP_OK) {
        if (required_size <= sizeof(buffer)) {
            err = nvs_get_str(ethernet_nvs_handle, "gateway", buffer, &required_size);
            if (err == ESP_OK) {
                //free(gateway);
                gateway = strdup(buffer); // Aloca memória e copia o valor
            }
        } else {
            ESP_LOGI(TAG, "Error to retrieve gateway");
        }
    }

    err = nvs_get_str(ethernet_nvs_handle, "netmask", NULL, &required_size);
    if (err == ESP_OK) {
        if (required_size <= sizeof(buffer)) {
            err = nvs_get_str(ethernet_nvs_handle, "netmask", buffer, &required_size);
            if (err == ESP_OK) {
                //free(netmask);
                netmask = strdup(buffer); // Aloca memória e copia o valor
            }
        } else {
            ESP_LOGI(TAG, "Error to retrieve netmask");
        }
    }

    err = nvs_get_str(ethernet_nvs_handle, "dns", NULL, &required_size);
    if (err == ESP_OK) {
        if (required_size <= sizeof(buffer)) {
            err = nvs_get_str(ethernet_nvs_handle, "dns", buffer, &required_size);
            if (err == ESP_OK) {
                //free(dns);
                dns = strdup(buffer); // Aloca memória e copia o valor
            }
        } else {
            ESP_LOGI(TAG, "Error to retrieve dns");
        }
    }

    ESP_LOGE("retrieve", "ip: %s", ip);
    ESP_LOGE("retrieve", "gateway: %s", gateway);
    ESP_LOGE("retrieve", "netmask: %s", netmask);
    ESP_LOGE("retrieve", "dns: %s", dns);

    // Fechar o namespace NVS
    nvs_close(ethernet_nvs_handle);
}

void ip_obtained(void){
    while (!ip_address_obtained) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void init_rede(void){
    esp_eth_stop(eth_handle_spi);
    if(strlen(ip) != 0){
        esp_netif_dhcpc_stop(eth_netif_spi); // Pare o cliente DHCP para evitar conflitos
        //Static IP defined 
        esp_netif_ip_info_t ip_info;
        memset(&ip_info, 0, sizeof(esp_netif_ip_info_t));
        ipaddr_aton(ip, &ip_info.ip.addr);
        ipaddr_aton(gateway, &ip_info.gw.addr);
        ipaddr_aton(netmask, &ip_info.netmask.addr);
        esp_netif_set_ip_info(eth_netif_spi, &ip_info); // Defina as informações de IP estático
        //no caso em IP não será estatico, remover as 6 linhas de código acima

        // Set DNS servers manually (8.8.8.8)
        esp_netif_dns_info_t dns_info;
        ipaddr_aton(dns, &dns_info.ip.u_addr.ip4);
        esp_netif_set_dns_info(eth_netif_spi, ESP_NETIF_DNS_MAIN, &dns_info);
        flag_ip = false;
    }
    else{
        flag_ip = true;
    }
    ESP_ERROR_CHECK(esp_eth_start(eth_handle_spi));
    set_message_ip(flag_ip);
    ip_obtained();
    status_ip(flag_ip);
}

void change_rede(char *ip, char *gateway, char *netmask, char *dns){
    store_ethernet_variables(ip, gateway, netmask, dns);
    retrieve_ethernet_variable();
    init_rede();
}

void change_ip(char *ip){
    store_ethernet_ip(ip);
    retrieve_ethernet_variable();
    init_rede();
}

void change_gateway(char *gateway){
    store_ethernet_gateway(gateway);
    retrieve_ethernet_variable();
    init_rede();
}

void change_netmask(char *netmask){
    store_ethernet_netmask(netmask);
    retrieve_ethernet_variable();
    init_rede();
}

void change_dns(char *dns){
    store_ethernet_dns(dns);
    retrieve_ethernet_variable();
    init_rede();
}

void nvs_erase(){
    nvs_flash_erase();
    init_rede();
    initialize_comunication();
}

void initialize_ethernet(void){
    // Configure SPI interface and Ethernet driver for specific SPI module
    /* start Ethernet driver state machine */
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    //inicializa o servico ISR (Interrupt Service Routine) para tratamento de interrupções

    // Initialize TCP/IP network interface (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Inicializa a interface de rede TCP/IP

    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    //Cria um loop de eventos padrão que é executado em segundo plano

    // Create instance(s) of esp-netif for SPI Ethernet(s)
    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();//configuracoes padroes da interface esp-netif SPI Ethernet
    esp_netif_config_t cfg_spi = { //configuracoes especificas da interface esp-netif SPI Ethernet
        .base = &esp_netif_config,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH
    };
    eth_netif_spi = NULL;
    char if_key_str[10];
    char if_desc_str[10];
    char num_str[3];
    itoa(0, num_str, 10);
    strcat(strcpy(if_key_str, "ETH_SPI_"), num_str);
    strcat(strcpy(if_desc_str, "eth"), num_str);
    esp_netif_config.if_key = if_key_str;
    esp_netif_config.if_desc = if_desc_str;
    esp_netif_config.route_prio = 30;
    eth_netif_spi = esp_netif_new(&cfg_spi);
    // Cria uma instancia de esp-netif para a interface SPI Ethernet

    // Init SPI bus
    spi_device_handle_t spi_handle = NULL;
    spi_bus_config_t buscfg = {
        .miso_io_num = CONFIG_EXAMPLE_ETH_SPI_MISO_GPIO,
        .mosi_io_num = CONFIG_EXAMPLE_ETH_SPI_MOSI_GPIO,
        .sclk_io_num = CONFIG_EXAMPLE_ETH_SPI_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(CONFIG_EXAMPLE_ETH_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    // Inicializa o barramento SPI para comunicação com o módulo Ethernet

    // Init specific SPI Ethernet module configuration from Kconfig (CS GPIO, Interrupt GPIO, etc.)
    spi_eth_module_config_t spi_eth_module_config;
    INIT_SPI_ETH_MODULE_CONFIG(spi_eth_module_config, 0);

    eth_handle_spi = NULL;

    spi_device_interface_config_t devcfg = {
        .command_bits = 16, // Actually it's the address phase in W5500 SPI frame
        .address_bits = 8,  // Actually it's the control phase in W5500 SPI frame
        .mode = 0,
        .clock_speed_hz = CONFIG_EXAMPLE_ETH_SPI_CLOCK_MHZ * 1000 * 1000,
        // Set SPI module Chip Select GPIO
        .spics_io_num = spi_eth_module_config.spi_cs_gpio,
        .queue_size = 20
    };

    ESP_ERROR_CHECK(spi_bus_add_device(CONFIG_EXAMPLE_ETH_SPI_HOST, &devcfg, &spi_handle));
    // Adiciona o dispositiov SPI ao barramento

    // w5500 ethernet driver is based on spi driver
    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(spi_handle);
    // Set remaining GPIO numbers and configuration used by the SPI module
    w5500_config.int_gpio_num = spi_eth_module_config.int_gpio;
    // Condigura o driver Ethernet W5500 com base no driver SPI.


    // Init MAC and PHY configs to default
    eth_mac_config_t mac_config_spi = ETH_MAC_DEFAULT_CONFIG();

    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.autonego_timeout_ms = 0; //  W5500 doesn't support auto-negotiation
    phy_config.phy_addr = spi_eth_module_config.phy_addr;
    phy_config.reset_gpio_num = spi_eth_module_config.phy_reset_gpio;
    // Configura as opções de MAC e PHY para seus valores padrão

    esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config_spi);;
    esp_eth_phy_t *phy_spi = esp_eth_phy_new_w5500(&phy_config);
    // Cria instâncias para a camada MAC e PHY para seus valores padrão

    esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(mac, phy_spi);
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config_spi, &eth_handle_spi));
    // Instala o driver Ethernet

    /* The SPI Ethernet module might not have a burned factory MAC address, we cat to set it manually.
    02:00:00 is a Locally Administered OUI range so should not be used except when testing on a LAN under your control.
    */
    ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle_spi, ETH_CMD_S_MAC_ADDR, (uint8_t[]) {
        0x02, 0x00, 0x00, 0x12, 0x34, 0x56
    }));
    // Define manualmente o endereço MAC para a interface Ethernet

    // attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif_spi, esp_eth_new_netif_glue(eth_handle_spi)));
    // Anexa o driver Ethernet à pilha TCP/IP

    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));
    // Registra manipuladores de eventos definidos pelo usuário
    
    retrieve_ethernet_variable();

    init_rede();
}
