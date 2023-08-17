idf_component_register(SRCS "bleuartServer.c"
                       INCLUDE_DIRS "." 
                        "${IDF_PATH}/components/bt/include/esp32/include"
                        "${IDF_PATH}/components/bt/host/bluedroid/api/include/api"
                        REQUIRES communication device eth)
