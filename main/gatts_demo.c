#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "mbedtls/aes.h"

#include "esp_mac.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "freertos/semphr.h"

#include "sdkconfig.h"
#include "ble_test.h"
#include "wifi_sta.h"
#include "mqtt_handle.h"
#include "uart_handle.h"
#include "datn_sntp.h"

#include "w25q32.h"

#define MAC_ADDR_SIZE   6

extern uint8_t wifi_connect;
extern uint8_t wifi_done;

extern volatile uint8_t data_flag;
extern uint8_t ble_data[100];
extern uint8_t mqtt_connect;
extern uint8_t mqtt_rcv_done;
extern uint8_t uart_rcv_done;
extern uint8_t mqtt_data[100];
extern char topic_pub[50];
extern esp_mqtt_client_handle_t client;
extern uart_event_t event;
extern uint8_t data[1024];
extern uint8_t uart_buffer[1024];
extern struct tm timeinfo;

QueueHandle_t ble_queue;
QueueHandle_t wifi_queue;

TaskHandle_t ble_task;
TaskHandle_t wifi_task;

uint8_t ble_start = 0;
volatile uint8_t fisrt_time_start = 1;

uint8_t wifi_state = 1;
uint8_t ssid[100];
uint8_t pass[100];

uint8_t init_mqtt = 0;

extern uint8_t w25q32_data[100];

SemaphoreHandle_t mutex1;               // for wifi

void ble_gatt_server();
void wifi_sta();
void uart2_event_task();

void app_main(void)
{   
    esp_err_t ret;
    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    init_spi_bus();

    uint8_t mac_bt[6] = {0};
    uint8_t mac_add[13];
    esp_efuse_mac_get_default(mac_bt);
    esp_read_mac(mac_bt, ESP_MAC_BT);
    sprintf((char*) mac_add, "%x%x%x%x%x%x", mac_bt[0], mac_bt[1], mac_bt[2], mac_bt[3], mac_bt[4], mac_bt[5]);
    mqtt_getMacAddress(mac_add);
    // ESP_ERROR_CHECK( ret );
    
    // ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));                     // need?
    // init_ble();

    ble_queue = xQueueCreate(1, sizeof(uint8_t));
    wifi_queue = xQueueCreate(1, sizeof(uint8_t));
    mutex1 = xSemaphoreCreateMutex();
    xTaskCreate(ble_gatt_server, "BLE Task", 4096, NULL, 3, NULL);
    xTaskCreate(wifi_sta, "Wifi Task", 4096, NULL, 3, NULL);
    xTaskCreatePinnedToCore(uart2_event_task, "UART2 Task", 2048, NULL, 2, NULL, 0);
    // return;
}

void ble_gatt_server()
{
    uint8_t ss[100] = "";
    w25q32_t check;
    ESP_LOGI("BLE:","ble task, wifi queue: %d, data_flag: %d", uxQueueMessagesWaiting(wifi_queue), data_flag);
    vTaskDelay(10);
    if (fisrt_time_start) {
        if (xSemaphoreTake(mutex1, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI("BLE:","ble task have mutex");
            check = W25Q32_check_wifi_info();
            // fisrt_time_start = 0;
            xSemaphoreGive(mutex1);
            ESP_LOGI("BLE:","ble task release mutex");
        }
    }
    vTaskDelay(10);
    wifi_init_lwip();
    if (!check.result) {
        if (xSemaphoreTake(mutex1, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI("BLE:","ble task have mutex");
            init_ble();
            ble_start = 1;
            xSemaphoreGive(mutex1);
            ESP_LOGI("BLE:","ble task release mutex");
        }
    }
    else {
        if (xSemaphoreTake(mutex1, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI("BLE:","ble task have mutex");
            W25Q32_get_wifi_info(check.i, check.l);
            strcpy((char*) ss, (char*) w25q32_data);
            get_wifi_info(ss, ssid, pass);
            mqtt_init();
            wifi_init_sta(ssid, pass);
            ESP_LOGI("BLE:","start connect wifi");
            xSemaphoreGive(mutex1);
            ESP_LOGI("BLE:","ble task release mutex");
        }
    }
    ESP_LOGI("BLE:","init ble");
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    // mqtt_init();
    // wifi_init_sta((uint8_t*) "Infrastructure NW", (uint8_t*) "NetworkPolicy");
    //mqtt_start();
    ESP_LOGI("BLE:","init lwip");
    for(;;) {
        if (data_flag) {
            if (xSemaphoreTake(mutex1, portMAX_DELAY) == pdTRUE) {
                ESP_LOGI("BLE:","ble task have mutex");
                data_flag = 0;
                strcpy((char*) ss, (char*) ble_data);
                get_wifi_info(ss, ssid, pass);
                mqtt_init();
                wifi_init_sta(ssid, pass);
                // mqtt_start();
                ESP_LOGI("BLE:","start connect wifi");
                xSemaphoreGive(mutex1);
                ESP_LOGI("BLE:","ble task release mutex");
            }
        }
        if (wifi_done) {
            if (xSemaphoreTake(mutex1, portMAX_DELAY) == pdTRUE) {
                ESP_LOGI("BLE:","ble task have mutex");
                wifi_done = 0;
                xQueueSend(wifi_queue, (void*) &wifi_connect, (TickType_t) 0);
                ESP_LOGI("BLE:","connect state = %d", wifi_connect);
                xSemaphoreGive(mutex1);
                ESP_LOGI("BLE:","ble task release mutex");
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);               // avoid esp32 reboot
    }



    // EXAMPLE
    // if (xSemaphoreTake(mutex1, portMAX_DELAY) == pdTRUE) {
    //     ESP_LOGI("BLE:","ble task have mutex");
    //     xQueueSend(ble_queue, (void*) ss, (TickType_t) 0);
    //     ESP_LOGI("BLE:","ble task send data\n");
    //     ESP_LOGI("BLE:","data:%s Available: %d", (char*) ss, uxQueueSpacesAvailable(wifi_queue));
    //     xSemaphoreGive(mutex1);
    //     ESP_LOGI("BLE:","ble task release mutex");
    // }
}
void wifi_sta() 
{
    uint8_t state;
    uint8_t is_dis = 0;                 // is disconect ble
    // wifi_init_lwip();
    ESP_LOGI("WIFI","wifi task, ble_queue: %d wifi_connect: %d", uxQueueMessagesWaiting(ble_queue), wifi_connect);
    for(;;) {
        if (uxQueueMessagesWaiting(wifi_queue) != 0) {
            if (xSemaphoreTake(mutex1, portMAX_DELAY) == pdTRUE) {
                ESP_LOGI("WIFI","wifi task have mutex");
                xQueueReceive(wifi_queue, &state, portMAX_DELAY);
                if (state) {                                    // connect successfuly
                    if (ble_start) {
                        ble_start = 0;
                        disable_ble();
                        W25Q32_write_wifi_info(ssid, pass, 1);
                        W25Q32_update_wifi_info(0, strlen((char*) ssid) + strlen((char*) pass) + 5);
                    }
                    ESP_LOGI("WIFI","disable ble");
                    is_dis = 1;
                    mqtt_start();
                    datn_sntp_init();
                    // if (mqtt_connect && strlen((char*) uart_buffer) != 0) {
                    //     datn_sntp_get_time();
                    //     uart_format_data(timeinfo, (char*) uart_buffer);
                    //     esp_mqtt_client_publish(client, "datn/aithing/data", (const char*) data, strlen((char*) data), 0, 0);
                    // }
                }
                else if (!state && is_dis) {                    // disable wifi when connect success before (need disable ble when connect success)
                    init_ble();
                    ESP_LOGI("WIFI","start ble again");
                    wifi_deinit_sta();
                    ESP_LOGI("WIFI","stop wifi connection");
                    is_dis = 0;
                    mqtt_stop();
                    datn_sntp_deinit();
                }
                else if (!state && !is_dis) {                   // connect fail in the first time
                    wifi_deinit_sta();
                    ESP_LOGI("WIFI","stop wifi connection, fail connect in the first time");
                    mqtt_stop();
                    datn_sntp_deinit();
                    if (fisrt_time_start) {
                        fisrt_time_start = 0;
                        init_ble();
                        ble_start = 1;
                    }
                }
                xSemaphoreGive(mutex1);
                ESP_LOGI("WIFI","wifi task release mutex");
            }
        }
        if (mqtt_rcv_done) {
            if (xSemaphoreTake(mutex1, portMAX_DELAY) == pdTRUE) {
                ESP_LOGI("WIFI","uart task have mutex");
                ESP_LOGI("WIFI","receive data from mqtt broker");
                mqtt_rcv_done = 0;
                uart_write_bytes(UART_NUM_2, (const char*) mqtt_data, strlen((char*) mqtt_data));
                ESP_LOGI("WIFI","uart send data: %s, len: %d", (const char*) mqtt_data, strlen((char*) mqtt_data));
                xSemaphoreGive(mutex1);
                ESP_LOGI("WIFI","uart task release mutex");
            }
        } 
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
        
        
    //EXAMPLE
    // if (xSemaphoreTake(mutex2, portMAX_DELAY) == pdTRUE) {
    //     ESP_LOGI("WIFI","wifi task have mutex");
    //     xQueueReceive(ble_queue, ssid, portMAX_DELAY);
    //     ESP_LOGI("WIFI","wifi task receive data!\n");
    //     ESP_LOGI("WIFI", "data:%s Available: %d", (char*) ssid, uxQueueSpacesAvailable(wifi_queue));
    //     xSemaphoreGive(mutex2);
    //     ESP_LOGI("WIFI","wifi task release mutex");
    // }    
}

void uart2_event_task()
{
    ESP_LOGI("UART","uart task");
    uart2_init();
    ESP_LOGI("UART","init uart");
    for(;;) {
        uart_handle_event();
        if (uart_rcv_done && wifi_connect) {
            if (xSemaphoreTake(mutex1, portMAX_DELAY) == pdTRUE) {
                ESP_LOGI("UART","uart task have mutex");
                ESP_LOGI("UART","receive data from uart");
                uart_rcv_done = 0;
                datn_sntp_get_time();
                uart_format_data(timeinfo, (char*) uart_buffer);
                esp_mqtt_client_publish(client, topic_pub, (const char*) data, strlen((char*) data), 0, 0);
                ESP_LOGI("UART","publish data to mqtt server, data: %s, len: %d", (const char*) data, event.size);
                memset(uart_buffer, 0, strlen((char*) uart_buffer));
                // cnt = 0;
                xSemaphoreGive(mutex1);
                ESP_LOGI("UART","uart task release mutex");
            }
        }
        else if (uart_rcv_done && !wifi_connect) {
            if (xSemaphoreTake(mutex1, portMAX_DELAY) == pdTRUE) {
                ESP_LOGI("UART","uart task have mutex");
                ESP_LOGI("UART","receive data from uart, but not connected to the internet yet");
                xSemaphoreGive(mutex1);
                ESP_LOGI("UART","uart task release mutex");
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}