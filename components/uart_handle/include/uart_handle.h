#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <time.h>
#include <sys/time.h>

#define TXD2 16             // connect mcu
#define RXD2 17

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

// static void uart2_event_task(void *pvParameters);
void uart2_init();
void uart_handle_event();
void uart_format_data(struct tm time, char *rx_buf);