idf_component_register(SRCS "mqtt_handle.c"
                    INCLUDE_DIRS "include"
                    REQUIRES mqtt
                    REQUIRES esp_wifi)
