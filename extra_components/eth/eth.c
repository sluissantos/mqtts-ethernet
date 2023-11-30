#include "eth.h"

static char *TAG = "eth_example";
char *namespace = "eth_namespace";

uint16_t cont_auto_ip = 0;

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
static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
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

void store_ethernet_one_variable(char *ident, char *param){
    nvs_handle_t ethernet_nvs_handle;

    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &ethernet_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening eth_namespace: %s", esp_err_to_name(err));
        return;
    }

    if(param != NULL){
        err = nvs_set_str(ethernet_nvs_handle, ident, param);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error to store %s, %s", ident, esp_err_to_name(err));
        }
        else{
            ESP_LOGE("store", "%s: %s", ident, param);
        }
    }

    // Efetivamente escrever as alterações no armazenamento permanente
    err = nvs_commit(ethernet_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS %s changes: %s", ident, esp_err_to_name(err));
    }

    nvs_close(ethernet_nvs_handle);
}

void store_ethernet_variables(char *ip, char *gateway, char *netmask, char *dns){
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
        else{
            ESP_LOGE("store", "ip: %s", ip);
        }
    }

    if(gateway != NULL){
        err = nvs_set_str(ethernet_nvs_handle, "gateway", gateway);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error to stage gateway, %s", esp_err_to_name(err));
        }
        else{
            ESP_LOGE("store", "gateway: %s", gateway);
        }
    }

    if(netmask != NULL){
        err = nvs_set_str(ethernet_nvs_handle, "netmask", netmask);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error to stage netmask: %s", esp_err_to_name(err));
        }
        else{
            ESP_LOGE("store", "netmask: %s", netmask);
        }
    }

    if(dns != NULL){
        err = nvs_set_str(ethernet_nvs_handle, "dns", dns);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Error to stage dns: %s", esp_err_to_name(err));
        }
        else{
            ESP_LOGE("store", "dns: %s", dns);
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
                ip = strdup(buffer); // Aloca memória e copia o valor
                ESP_LOGE("retrieve", "ip: %s", ip);
            }
            else {
                ESP_LOGI(TAG, "Error to retrieve ip");
            }
        }
    }

    err = nvs_get_str(ethernet_nvs_handle, "gateway", NULL, &required_size);
    if (err == ESP_OK) {
        if (required_size <= sizeof(buffer)) {
            err = nvs_get_str(ethernet_nvs_handle, "gateway", buffer, &required_size);
            if (err == ESP_OK) {
                gateway = strdup(buffer); // Aloca memória e copia o valor
                ESP_LOGE("retrieve", "gateway: %s", gateway);   
            }
            else {
                ESP_LOGI(TAG, "Error to retrieve gateway");
            }
        }
    }

    err = nvs_get_str(ethernet_nvs_handle, "netmask", NULL, &required_size);
    if (err == ESP_OK) {
        if (required_size <= sizeof(buffer)) {
            err = nvs_get_str(ethernet_nvs_handle, "netmask", buffer, &required_size);
            if (err == ESP_OK) {
                netmask = strdup(buffer); // Aloca memória e copia o valor
                ESP_LOGE("retrieve", "netmask: %s", netmask);
            }
            else {
                ESP_LOGI(TAG, "Error to retrieve netmask");
            }
        }
    }

    err = nvs_get_str(ethernet_nvs_handle, "dns", NULL, &required_size);
    if (err == ESP_OK) {
        if (required_size <= sizeof(buffer)) {
            err = nvs_get_str(ethernet_nvs_handle, "dns", buffer, &required_size);
            if (err == ESP_OK) {
                dns = strdup(buffer); // Aloca memória e copia o valor
                ESP_LOGE("retrieve", "dns: %s", dns);
            }
            else {
                ESP_LOGI(TAG, "Error to retrieve dns");
            }
        }
    }

    // Fechar o namespace NVS
    nvs_close(ethernet_nvs_handle);
}

char* retrieve_ethernet_one_variable(char *variable) {
    nvs_handle_t ethernet_nvs_handle;

    esp_err_t err = nvs_open(namespace, NVS_READONLY, &ethernet_nvs_handle);
    if (err != ESP_OK) {
        //ESP_LOGI(TAG, "Error to open eth_namespace to retrieve");
        return NULL;
    }

    char buffer[256]; // Buffer para armazenar as strings recuperadas
    char *var = NULL;

    // Recuperar as variáveis
    size_t required_size;
    err = nvs_get_str(ethernet_nvs_handle, variable, NULL, &required_size);
    if (err == ESP_OK) {
        if (required_size <= sizeof(buffer)) {
            err = nvs_get_str(ethernet_nvs_handle, variable, buffer, &required_size);
            if (err == ESP_OK) {
                var = strdup(buffer); // Aloca memória e copia o valor
                //ESP_LOGE("retrieve", "variable: %s", variable);
            }
            else {
                ESP_LOGI(TAG, "Error to retrieve variable");
            }
        }
    }
    // Fechar o namespace NVS
    nvs_close(ethernet_nvs_handle);
    return var;
}

void ip_obtained(void){
    while (!ip_address_obtained) {
        cont_auto_ip++;
        if(cont_auto_ip == 2000){
            esp_restart();
        }
        vTaskDelay(50);
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
    if(flag_ip){
        ip_obtained();
    }
    status_ip(flag_ip);
}

void change_rede(char *ip, char *gateway, char *netmask, char *dns){
    store_ethernet_variables(ip, gateway, netmask, dns);
    retrieve_ethernet_variable();
    esp_restart();
}

void change_one_rede_variable(char* ident, char *param){
    store_ethernet_one_variable(ident, param);
    retrieve_ethernet_variable();
    esp_restart();
}

void initialize_ethernet(const uint8_t maceth[6]){
    ESP_ERROR_CHECK(example_eth_init(&eth_handle_spi));

    // Initialize TCP/IP network interface aka the esp-netif (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
    esp_netif_config_t cfg_spi = {
        .base = &esp_netif_config,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH
    };
    char if_key_str[10];
    char if_desc_str[10];
    char num_str[3];
    itoa(0, num_str, 10);
    strcat(strcpy(if_key_str, "ETH_"), num_str);
    strcat(strcpy(if_desc_str, "eth"), num_str);
    esp_netif_config.if_key = if_key_str;
    esp_netif_config.if_desc = if_desc_str;
    esp_netif_config.route_prio -= 0*5;
    esp_netif_t *eth_netif = esp_netif_new(&cfg_spi);

    ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle_spi, ETH_CMD_S_MAC_ADDR, maceth));

    // Attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle_spi)));

    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    ESP_ERROR_CHECK(esp_eth_start(eth_handle_spi));
    retrieve_ethernet_variable();

    init_rede();
}
