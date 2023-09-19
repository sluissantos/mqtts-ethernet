idf_component_register(SRCS "communication.c"
                       INCLUDE_DIRS "."
                       REQUIRES uart eth mqtts_eth)
