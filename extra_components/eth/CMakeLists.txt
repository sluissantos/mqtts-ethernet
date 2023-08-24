idf_component_register(SRCS "eth.c"
                       INCLUDE_DIRS "."
                       "${IDF_PATH}/components/nvs_flash/include"
                       "${IDF_PATH}/components/spi_flash/include"
                       REQUIRES communication mqtts_eth)
