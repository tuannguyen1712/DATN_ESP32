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

extern volatile uint8_t data_flag;
extern uint8_t ble_data[100];

QueueHandle_t ble_queue;
QueueHandle_t wifi_queue;

TaskHandle_t ble_task;
TaskHandle_t wifi_task;

uint8_t wifi_state = 1;

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
    ble_queue = xQueueCreate(1, sizeof(uint8_t) * 100);
    wifi_queue = xQueueCreate(1, sizeof(uint8_t));
    mutex1 = xSemaphoreCreateMutex();
    mutex2 = xSemaphoreCreateMutex();
    xTaskCreate(ble_gatt_server, "BLE Task", 4096, NULL, 1, NULL);
    xTaskCreate(wifi_sta, "Wifi Task", 4096, NULL, 1, NULL);
    // xTaskCreate(wifi_sta, "Wifi Task", 2048, NULL, 1, NULL);
    // vTaskStartScheduler();
    // return;
}

void ble_gatt_server()
{
    uint8_t wifi_flag = 1;
    uint8_t ss[100] = "";
    uint8_t connect_state;
    uint8_t ssid[100];
    uint8_t pass[100];
    uint8_t data[100];
    ESP_LOGI("BLE:","ble task, wifi queue: %d, data_flag: %d", uxQueueMessagesWaiting(wifi_queue), data_flag);
    init_ble();
    wifi_init_lwip();
    while (1) {
        if (data_flag) {
            if (xSemaphoreTake(mutex1, portMAX_DELAY) == pdTRUE) {
                ESP_LOGI("BLE:","ble task have mutex");
                data_flag = 0;
                strcpy((char*) ss, (char*) ble_data);
                get_wifi_info(ss, ssid, pass);
                wifi_init_sta(ssid, pass);
                xQueueSend(ble_queue, (void*) ss, (TickType_t) 2);
                ESP_LOGI("BLE:","send data to queue");
                disable_ble();
                ESP_LOGI("BLE:","disable ble");
                xSemaphoreGive(mutex1);
                ESP_LOGI("BLE:","ble task release mutex");
            }
        }
        if (uxQueueMessagesWaiting(wifi_queue)) {
            if (xSemaphoreTake(mutex1, portMAX_DELAY) == pdTRUE) {
                ESP_LOGI("BLE:","ble task have mutex");
                xQueueReceive(wifi_queue, &connect_state, portMAX_DELAY);
                ESP_LOGI("BLE:","received data from wifi_queue");
                init_ble();
                ESP_LOGI("BLE:","enable ble");
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
    // uint8_t ssid[100];
    // uint8_t pass[100];
    // uint8_t data[100];
    uint8_t connect_state;
    // wifi_init_lwip();
    ESP_LOGI("WIFI","wifi task, ble_queue: %d wifi_connect: %d", uxQueueMessagesWaiting(ble_queue), wifi_connect);
    while (1) {
        // if (uxQueueMessagesWaiting(ble_queue) != 0) {
        //     if (xSemaphoreTake(mutex1, portMAX_DELAY) == pdTRUE) {
        //         ESP_LOGI("WIFI","wifi task have mutex");
        //         xQueueReceive(ble_queue, data, portMAX_DELAY); 
        //         ESP_LOGI("WIFI", "receive data from ble: %s", data);
        //         get_wifi_info(data, ssid, pass);
        //         ESP_LOGI("WIFI", "ssid:%s password:%s", (char*) ssid, (char*) pass);
        //         // wifi_init_lwip();
        //         // wifi_init_sta(ssid, pass);
        //         //mqtt_app_start();
        //         xSemaphoreGive(mutex1);
        //         ESP_LOGI("WIFI","wifi task release mutex");
        //     }
        // }
        if (wifi_connect == 0) {
            if (xSemaphoreTake(mutex1, portMAX_DELAY) == pdTRUE) {
                ESP_LOGI("WIFI","wifi task have mutex");
                wifi_connect = 1;
                connect_state = 0;
                xQueueSend(wifi_queue, (void*) &connect_state, (TickType_t) 0);
                ESP_LOGI("WIFI", "send wifi connect state");
                wifi_deinit_sta();
                ESP_LOGI("WIFI", "disable wifi");
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