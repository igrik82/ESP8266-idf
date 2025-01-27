#include "mqtt.h"
#include "esp_log.h"
#include "mqtt_client.h"

extern EventGroupHandle_t common_event_group;
extern uint8_t _wifi_connect_bit;
extern uint8_t _wifi_disconnect_bit;
extern uint8_t _wifi_got_ip;
extern uint8_t _wifi_lost_ip;

namespace Mqtt_NS {

// Static variables
esp_mqtt_client_config_t Mqtt::mqtt_cfg {};
Mqtt::state_m Mqtt::_state { Mqtt::state_m::NOT_INITIALISED };

Mqtt::Mqtt(void)
{
    if (_state == state_m::NOT_INITIALISED) {
        mqtt_cfg.uri = CONFIG_BROKER_URL;
        mqtt_cfg.port = CONFIG_BROKER_PORT;
        mqtt_cfg.client_id = CONFIG_CLIENT_ID;
        mqtt_cfg.username = CONFIG_MQTT_USER;
        mqtt_cfg.password = CONFIG_MQTT_PASSWORD;
        mqtt_cfg.keepalive = CONFIG_MQTT_KEEP_ALIVE;
        mqtt_cfg.protocol_ver = MQTT_PROTOCOL_V_3_1_1;
        ESP_LOGI(TAG, "MQTT client init");
        client = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler,
            NULL);

        _state = state_m::INITIALISED;
    }
    for (;;) {
        EventBits_t bits = xEventGroupWaitBits(common_event_group,
            _wifi_got_ip | _wifi_disconnect_bit,
            pdFALSE, pdFALSE, portMAX_DELAY);
        if (bits & _wifi_got_ip) {
            ESP_LOGI(TAG, "[APP] Startup..");
            ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
            esp_mqtt_client_start(client);
            ESP_LOGI(TAG, "Client started");
            xEventGroupClearBits(common_event_group, _wifi_got_ip);
        } else if (bits & _wifi_disconnect_bit) {
            // BUG: If stopped it crash
            xEventGroupClearBits(common_event_group, _wifi_disconnect_bit);
            // esp_mqtt_client_stop(client);
            // esp_mqtt_client_destroy(client);
            // _state = state_m::NOT_INITIALISED;
            ESP_LOGI(TAG, "Client stopped");
        }
    }
}

esp_err_t Mqtt::mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    ESP_LOGI(TAG, "in event %d", event->event_id);
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
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
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}
void Mqtt::mqtt_event_handler(void* handler_args, esp_event_base_t base,
    int32_t event_id, void* event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base,
        event_id);
    mqtt_event_handler_cb((esp_mqtt_event_handle_t)event_data);
}

void Mqtt::start(void) { }

} // namespace Mqtt_NS
