idf_component_register(SRCS "ble_test.c"
                    INCLUDE_DIRS "include"
                    REQUIRES bt
                    REQUIRES wifi_sta)
