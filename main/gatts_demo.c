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

extern uint8_t wifi_connect;
extern uint8_t wifi_done;

extern volatile uint8_t data_flag;
extern uint8_t ble_data[100];

QueueHandle_t ble_queue;
QueueHandle_t wifi_queue;

TaskHandle_t ble_task;
TaskHandle_t wifi_task;

uint8_t wifi_state = 1;

uint8_t init_mqtt = 0;

void ble_gatt_server();
void wifi_sta();
SemaphoreHandle_t mutex1;               // for wifi
SemaphoreHandle_t mutex2;               // for ble

void app_main(void)
{
    esp_err_t ret;
    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    // ESP_ERROR_CHECK( ret );
    
    // ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));                     // need?
    // init_ble();
    ble_queue = xQueueCreate(1, sizeof(uint8_t));
    wifi_queue = xQueueCreate(1, sizeof(uint8_t));
    mutex1 = xSemaphoreCreateMutex();
    mutex2 = xSemaphoreCreateMutex();
    xTaskCreate(ble_gatt_server, "BLE Task", 5120, NULL, 1, NULL);
    xTaskCreate(wifi_sta, "Wifi Task", 5120, NULL, 1, NULL);
    // xTaskCreate(wifi_sta, "Wifi Task", 2048, NULL, 1, NULL);
    // vTaskStartScheduler();
    // return;
}

void ble_gatt_server()
{
    uint8_t wifi_flag = 1;
    uint8_t ss[100] = "";
    uint8_t ssid[100];
    uint8_t pass[100];
    uint8_t connect_state;
    ESP_LOGI("BLE:","ble task, wifi queue: %d, data_flag: %d", uxQueueMessagesWaiting(wifi_queue), data_flag);
    init_ble();
    ESP_LOGI("BLE:","init ble");
    wifi_init_lwip();
    mqtt_init();
    wifi_init_sta((uint8_t*) "Infrastructure NW", (uint8_t*) "NetworkPolicy");
    ESP_LOGI("BLE:","init lwip");
    while (1) {
        if (data_flag) {
            if (xSemaphoreTake(mutex1, portMAX_DELAY) == pdTRUE) {
                ESP_LOGI("BLE:","ble task have mutex");
                data_flag = 0;
                strcpy((char*) ss, (char*) ble_data);
                get_wifi_info(ss, ssid, pass);
                // wifi_init_sta(ssid, pass);
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
        vTaskDelay(50 / portTICK_PERIOD_MS);
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
    while (1) {
        if (uxQueueMessagesWaiting(wifi_queue) != 0) {
            if (xSemaphoreTake(mutex1, portMAX_DELAY) == pdTRUE) {
                ESP_LOGI("WIFI","wifi task have mutex");
                xQueueReceive(wifi_queue, &state, portMAX_DELAY);
                if (state) {                                    // connect successfuly
                    disable_ble();
                    ESP_LOGI("WIFI","disable ble");
                    mqtt_start();
                    is_dis = 1;
                }
                else if (!state && is_dis) {                    // disable wifi when connect success before (need disable ble when connect success)
                    init_ble();
                    ESP_LOGI("WIFI","start ble again");
                    wifi_deinit_sta();
                    ESP_LOGI("WIFI","stop wifi connection");
                    is_dis = 0;
                }
                else if (!state && !is_dis) {                   // connect fail in the first time
                    wifi_deinit_sta();
                    ESP_LOGI("WIFI","stop wifi connection, fail connect in the first time");
                }
                xSemaphoreGive(mutex1);
                ESP_LOGI("WIFI","wifi task release mutex");
            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
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