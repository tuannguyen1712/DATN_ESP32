/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/****************************************************************************
*
* This demo showcases BLE GATT server. It can send adv data, be connected by client.
* Run the gatt_client demo, the client demo will automatically connect to the gatt_server demo.
* Client demo will enable gatt_server's notify after connection. The two devices will then exchange
* data.
*
****************************************************************************/


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
#include "semphr.h"

#include "sdkconfig.h"
#include "ble_test.h"
#include "wifi_sta.h"

QueueHandle_t ble_queue;
QueueHandle_t wifi_queue;

void ble_gatt_server();
void wifi_sta();
SemaphoreHandle_t mutex;

void app_main(void)
{
    esp_err_t ret;
    // Initialize NVS.
    ret = nvs_flash_init();
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    //     ESP_ERROR_CHECK(nvs_flash_erase());
    //     ret = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK( ret );
    
    // ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));                     // need?
    // init_ble();
    xTaskCreate(ble_gatt_server, "BLE Task", 4096, NULL, 1, NULL);
    xTaskCreate(wifi_sta, "Wifi Task", 2048, NULL, 1, NULL);
    // return;
}

void ble_gatt_server()
{
    uint8_t wifi_flag = 1;
    init_ble();
    ble_queue = xQueueCreate(2, sizeof(uint8_t) * 100);
    wifi_queue = xQueueCreate(1, sizeof(uint8_t));
    while (1) {
        
    }
}

void wifi_sta();