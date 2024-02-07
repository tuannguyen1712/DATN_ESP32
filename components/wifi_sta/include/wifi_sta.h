#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define WIFI_CONNECTED_BIT      1
#define WIFI_FAIL_BIT           2

#define WIFI_CONNECT_SUCCESS    1
#define WIFI_CONNECT_FAIL       0
#define MAXIMUM_RETRY           3

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);     
void wifi_init_sta(uint8_t *s, uint8_t *p);
void wifi_deinit_sta();

void wifi_init_lwip();
void get_wifi_info(uint8_t *info, uint8_t *ssid, uint8_t *password);