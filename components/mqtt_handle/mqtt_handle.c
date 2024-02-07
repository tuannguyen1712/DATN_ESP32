#include <stdio.h>
#include "mqtt_handle.h"

static const char *TAG = "MQTT";
esp_mqtt_client_handle_t client;
char topic[] = "doan2/aithing/data";

void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        int msg_id = esp_mqtt_client_subscribe(client, "doan2/aithing/control", 2);
	    ESP_LOGI(TAG, "Subcribe to topic: doan2/aithing/control , id: %d", msg_id);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        esp_mqtt_client_publish(client, "datn/aithing/data", (const char*) "OK", 2, 0, 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
//        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        // loại bỏ ký tự thừa từ message
        char *data = (char *)malloc(event->data_len + 1);
	    memcpy(data, event->data + event->current_data_offset, event->data_len);
        char x[100];

        itoa(event->data_len, x, 10);
	    data[event->data_len] = 0;

        esp_mqtt_client_publish(client, "datn/aithing/data", (const char*) "OK", 2, 0, 0);
        // uart_write_bytes(UART_NUM_2, (const char*) data, event->data_len);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}


void mqtt_app_start()
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://broker.emqx.io:1883",
        // .broker.address.uri = "mqtt://white-dev.aithings.vn:1883",
        // .broker.address.hostname = "192.168.1.1",
        // .broker.address.port = 18083
        // .broker.address.hostname = "localhost",
        // .broker.address.port = 18083,
        // .broker.address.transport = MQTT_TRANSPORT_OVER_TCP,
        // .broker.address.path = "/mqtt",
        .session.keepalive = 60
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}