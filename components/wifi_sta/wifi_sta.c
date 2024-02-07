#include <stdio.h>
#include "wifi_sta.h"

uint8_t wifi_connect = 1;       // = 0 when connect fail
esp_netif_t *ptr;
int s_retry_num = 0;
esp_event_handler_instance_t g_instance_any_id;
esp_event_handler_instance_t g_instance_got_ip;
    
void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI("WIFI STATION", "retry to connect to the AP");
        } else {
            ESP_LOGI("WIFI STATION", "Can't connect to the AP point");
            s_retry_num = 0;
            wifi_connect = 0;
        }
        ESP_LOGI("WIFI STATION","connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("WIFI STATION", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connect = 1;
        s_retry_num = 0;
    }
}

void wifi_init_sta(uint8_t *s, uint8_t *p)
{
    wifi_config_t wifi_config;
    strcpy((char*) wifi_config.sta.ssid, (char*) s);
    strcpy((char*) wifi_config.sta.password, (char*) p);
    wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("WIFI STATION", "wifi_init_sta finished.");
}

void wifi_deinit_sta() {
    esp_wifi_disconnect();
    esp_wifi_stop();
    // esp_wifi_deinit();

    // esp_netif_destroy(ptr); // Hủy instance của netif
    // esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, g_instance_got_ip); // Hủy đăng ký sự kiện IP_EVENT_STA_GOT_IP
    // esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, g_instance_any_id); // Hủy đăng ký sự kiện WIFI_EVENT
    // esp_event_loop_delete_default();
}

// SemaphoreHandle_t mutex; // declare Sermaphore variable
// xSemaphoreTake           // get sermaphore
// xSemaphoreGive

void wifi_init_lwip() {
    wifi_connect = 1;
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ptr = esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    g_instance_any_id = instance_any_id;                                                    
    g_instance_got_ip = instance_got_ip;
}

void get_wifi_info(uint8_t *info, uint8_t *ssid, uint8_t *password)
{
    uint8_t sfl = 1;
    int k = 0;
    for (int i = 2; i < strlen((char*) info); i++) {
        if (sfl) {
            if (info[i] != '\t') {
                ssid[k] = info[i];
                k++;
            }
            else {
                sfl = 0;
                ssid[k] = 0;
                k = 0;
            }
        }
        else {
            strcpy((char*) password, (char*) (info + i + 2));
            break;
        }
    }
    printf("ssid: .%s. pass: .%s.\n", ssid, password);
}