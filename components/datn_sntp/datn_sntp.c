#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"

time_t now;
struct tm timeinfo;

void datn_sntp_init() 
{
    ESP_LOGI("SNTP", "Initializing and starting SNTP");
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    config.renew_servers_after_new_IP = true;       // update SNTP server when device have new ip addreess
    esp_netif_sntp_init(&config);
    
    // wait for time to be set
    time_t now = 0;
    int retry = 0;
    const int retry_count = 15;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
        ESP_LOGI("SNTP", "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}

void datn_sntp_get_time() 
{
    time(&now);
    setenv("TZ", "CST-7", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    timeinfo.tm_year += 1900;
    timeinfo.tm_mon += 1;
    ESP_LOGI("SNTP", "Current time in Vietnam: %d:%02d:%02d %02d:%02d:%02d", timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday,
                                                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

void datn_sntp_deinit() 
{
    esp_netif_sntp_deinit();
}