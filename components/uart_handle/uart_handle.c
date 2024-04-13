#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "uart_handle.h"

static QueueHandle_t uart2_queue;
uint8_t uart_rcv_done = 0;

uint32_t last_tick = 0;
uint32_t g_sys_tick = 0;
uart_event_t event;
uint8_t dtmp[1024];
uint8_t data[1024];
uint8_t uart_buffer[1024];
uint8_t cnt = 0;

// delete pattern det and buffered_size;
// static void uart2_event_task(void *pvParameters)
// {
//     uart_event_t event;
//     uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
//     for(;;) {
//         //Waiting for UART event.
//         if(xQueueReceive(uart2_queue, (void * )&event, (TickType_t)portMAX_DELAY)) {
//             bzero(dtmp, RD_BUF_SIZE);
//             ESP_LOGI("UART2", "uart[%d] event:", UART_NUM_2);
//             switch(event.type) {
//                 //Event of UART receving data
//                 /*We'd better handler data event fast, there would be much more data events than
//                 other types of events. If we take too much time on data event, the queue might
//                 be full.*/
//                 case UART_DATA:
//                     ESP_LOGI("UART2", "[UART DATA]: %d", event.size);
//                     uart_read_bytes(UART_NUM_2, dtmp, event.size, portMAX_DELAY);
//                     uart_rcv_done = 1;
//                     break;
//                 //Event of HW FIFO overflow detected
//                 case UART_FIFO_OVF:
//                     ESP_LOGI("UART2", "hw fifo overflow");
//                     // If fifo overflow happened, you should consider adding flow control for your application.
//                     // The ISR has already reset the rx FIFO,
//                     // As an example, we directly flush the rx buffer here in order to read more data.
//                     uart_flush_input(UART_NUM_2);
//                     xQueueReset(uart2_queue);
//                     break;
//                 //Event of UART ring buffer full
//                 case UART_BUFFER_FULL:
//                     ESP_LOGI("UART2", "ring buffer full");
//                     // If buffer full happened, you should consider increasing your buffer size
//                     // As an example, we directly flush the rx buffer here in order to read more data.
//                     uart_flush_input(UART_NUM_2);
//                     xQueueReset(uart2_queue);
//                     break;
//                 //Event of UART RX break detected
//                 case UART_BREAK:
//                     ESP_LOGI("UART2", "uart rx break");
//                     break;
//                 //Event of UART parity check error
//                 case UART_PARITY_ERR:
//                     ESP_LOGI("UART2", "uart parity error");
//                     break;
//                 //Event of UART frame error
//                 case UART_FRAME_ERR:
//                     ESP_LOGI("UART2", "uart frame error");
//                     break;
//                 //Others
//                 default:
//                     ESP_LOGI("UART2", "uart event type: %d", event.type);
//                     break;
//             }
//         }
//     }
//     free(dtmp);
//     dtmp = NULL;
//     vTaskDelete(NULL);
// }

void periodic_timer_callback(void* arg) {
    g_sys_tick++;
    if (g_sys_tick >= 0xFFFFFFFF) 
        g_sys_tick = 0;
}

void tim_start() {
    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .name = "periodic"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 999));
}

void uart2_init()
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    tim_start();

    uart_driver_install(UART_NUM_2, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart2_queue, 0);
    uart_param_config(UART_NUM_2, &uart_config);                    // connect stm32
    uart_set_pin(UART_NUM_2, TXD2, RXD2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_pattern_queue_reset(UART_NUM_2, 20);
    // xTaskCreatePinnedToCore(uart2_event_task, "UART2 Task", 2048, NULL, 3, NULL, 1);
}

void uart_handle_event() 
{
    if(xQueueReceive(uart2_queue, (void * )&event, (TickType_t)portMAX_DELAY)) {
        bzero(dtmp, RD_BUF_SIZE);
        ESP_LOGI("UART2", "uart[%d] event:", UART_NUM_2);
        switch(event.type) {
            //Event of UART receving data
            /*We'd better handler data event fast, there would be much more data events than
            other types of events. If we take too much time on data event, the queue might
            be full.*/
            case UART_DATA:
                uart_read_bytes(UART_NUM_2, dtmp, event.size, portMAX_DELAY);
                ESP_LOGI("UART2", "[UART DATA]: %d", event.size);
                // cnt++;
                // if (cnt == 1) {
                //     memset(uart_buffer, 0, sizeof(uart_buffer));
                //     strcpy((char*) uart_buffer, (char*) dtmp);
                // }
                // else if (cnt == 2) {
                //     strcat((char*) uart_buffer, (char*) dtmp);
                //     uart_rcv_done = 1;
                //     cnt = 0;
                // }
                if (g_sys_tick - last_tick > 10) {
                    ESP_LOGI("UART2", "turn 1");
                    memset(uart_buffer, 0, sizeof(uart_buffer));
                    strcpy((char*) uart_buffer, (char*) dtmp);
                }
                else {
                    ESP_LOGI("UART2", "turn 2");
                    strcat((char*) uart_buffer, (char*) dtmp);
                    uart_rcv_done = 1;
                }
                last_tick = g_sys_tick;
                break;
            //Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
                ESP_LOGI("UART2", "hw fifo overflow");
                // If fifo overflow happened, you should consider adding flow control for your application.
                // The ISR has already reset the rx FIFO,
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input(UART_NUM_2);
                xQueueReset(uart2_queue);
                break;
            //Event of UART ring buffer full
            case UART_BUFFER_FULL:
                ESP_LOGI("UART2", "ring buffer full");
                // If buffer full happened, you should consider increasing your buffer size
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input(UART_NUM_2);
                xQueueReset(uart2_queue);
                break;
            //Event of UART RX break detected
            case UART_BREAK:
                ESP_LOGI("UART2", "uart rx break");
                break;
            //Event of UART parity check error
            case UART_PARITY_ERR:
                ESP_LOGI("UART2", "uart parity error");
                break;
            //Event of UART frame error
            case UART_FRAME_ERR:
                ESP_LOGI("UART2", "uart frame error");
                break;
            //Others
            default:
                ESP_LOGI("UART2", "uart event type: %d", event.type);
                break;
        }
    }
}

void uart_format_data(struct tm time, char *rx_buf)
{
    sprintf((char*) data, "%d%02d%02d%02d%02d%02d\t%s", time.tm_year, time.tm_mon, time.tm_mday,
                                                time.tm_hour, time.tm_min, time.tm_sec, rx_buf);
}