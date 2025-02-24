#include "mqtt.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "portmacro.h"

// External variables from main
extern EventGroupHandle_t common_event_group;
extern uint8_t _wifi_connect_bit;
extern uint8_t _wifi_disconnect_bit;
extern uint8_t _wifi_got_ip;
extern uint8_t _wifi_lost_ip;

namespace Mqtt_NS {

// Static variables
esp_mqtt_client_config_t Mqtt::mqtt_cfg {};
Mqtt::state_m Mqtt::_state { Mqtt::state_m::NOT_INITIALISED };

Mqtt::Mqtt(QueueHandle_t& temperature_queue)
{
    // Queue from sensor
    _sensor_queue = &temperature_queue;

    // Client initialisation
    if (_state == state_m::NOT_INITIALISED) {
        mqtt_cfg.uri = CONFIG_BROKER_URL;
        mqtt_cfg.port = CONFIG_BROKER_PORT;
        mqtt_cfg.client_id = CONFIG_CLIENT_ID;
        mqtt_cfg.username = MQTT_USER;
        mqtt_cfg.password = MQTT_PASSWORD;
        mqtt_cfg.keepalive = CONFIG_MQTT_KEEP_ALIVE;
        mqtt_cfg.protocol_ver = MQTT_PROTOCOL_V_3_1_1;
        ESP_LOGI(TAG, "MQTT client init");
        client = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler,
            NULL);

        _state = state_m::INITIALISED;
    }
}

// Evens callback
esp_err_t Mqtt::mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    ESP_LOGI(TAG, "in event %d", event->event_id);
    // esp_mqtt_client_handle_t client = event->client;
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connected to MQTT broker at %s:%d", mqtt_cfg.uri,
            mqtt_cfg.port);
        ESP_LOGI(TAG, "Login - \"%s\" password - \"%s\"", mqtt_cfg.username,
            mqtt_cfg.password);
        // msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1,
        // 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Disconnected from MQTT broker at %s:%d", mqtt_cfg.uri,
            mqtt_cfg.port);
        // Client cannot be stopped from task!
        // esp_mqtt_client_stop(client);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        break;
    case MQTT_EVENT_DATA:
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

// Events handler
void Mqtt::mqtt_event_handler(void* handler_args, esp_event_base_t base,
    int32_t event_id, void* event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base,
        event_id);
    mqtt_event_handler_cb((esp_mqtt_event_handle_t)event_data);
}

// Start MQTT client
void Mqtt::start()
{
    // wait for WI-FI connection and IP
    EventBits_t bits = xEventGroupWaitBits(common_event_group,
        _wifi_got_ip | _wifi_disconnect_bit,
        pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & _wifi_got_ip) {
        ESP_LOGI(TAG, "MQTT Startup..");
        // ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
        esp_mqtt_client_start(client);
        ESP_LOGI(TAG, "Client MQTT started");
        xEventGroupClearBits(common_event_group, _wifi_got_ip);

        // Publish sensor data
        SensorData_t sensor_data = {};
        for (;;) {
            if (xQueueReceive(*_sensor_queue, &sensor_data, portMAX_DELAY) == pdTRUE) {

                if (sensor_data.temperature == 0.0f && sensor_data.sensor_id == 0) {
                    ESP_LOGW(TAG, "Получено нулевое значение от датчика %d",
                        sensor_data.sensor_id);
                    continue; // Пропустить отправку
                }
                char msg[10]; // buffer for message
                snprintf(msg, sizeof(msg), "%.2f", sensor_data.temperature);

                char topic[50]; // buffer for topic
                snprintf(topic, sizeof(topic), "/topic/%d", sensor_data.sensor_id);

                ESP_LOGI(TAG, "Temperature from MQTT: %s %s", msg, topic);
                esp_mqtt_client_publish(client, topic, msg, 0, 0, 0);
            }
            // vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

} // namespace Mqtt_NS
