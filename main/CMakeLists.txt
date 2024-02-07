idf_component_register(SRCS "gatts_demo.c"
                    INCLUDE_DIRS "."
                    REQUIRES ble_test
                    REQUIRES nvs_flash
                    REQUIRES wifi_sta
                    REQUIRES freertos
                    REQUIRES mqtt_handle)
