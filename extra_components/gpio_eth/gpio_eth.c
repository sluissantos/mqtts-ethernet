#include "gpio_eth.h"

void eth_reset(){
    gpio_set_level(W5500_RESET_PIN, 0); // Nível baixo para redefinir o módulo.
    vTaskDelay(pdMS_TO_TICKS(100)); // Aguarde um curto período de tempo.
    gpio_set_level(W5500_RESET_PIN, 1); // Nível alto para operação normal.
    esp_restart();
}

void init_gpio() {
    // Configuração do pino de reset
    gpio_config_t reset_io_config;
    reset_io_config.intr_type = GPIO_PIN_INTR_DISABLE;
    reset_io_config.mode = GPIO_MODE_OUTPUT;
    reset_io_config.pin_bit_mask = (1ULL << W5500_RESET_PIN);
    reset_io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    reset_io_config.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&reset_io_config);

    vTaskDelay(pdMS_TO_TICKS(50)); // Aguarde um curto período de tempo.

    gpio_set_level(W5500_RESET_PIN, 1); // Nível alto para operação normal.
}
