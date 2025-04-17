#include "mqtt.h"
#include <cstdio>

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

bool Mqtt::find_mqtt_server(MdnsMqttServer_t& mqtt_server)
{

    ESP_ERROR_CHECK(mdns_init());

    ESP_LOGI("MDNS", "Querying for MQTT servers...");
    mdns_result_t* results = NULL;
    esp_err_t err = mdns_query_ptr("_mqtt", "_tcp", 3000, 20, &results);

    if (err != ESP_OK || results == NULL) {
        ESP_LOGE("MDNS", "Failed to find MQTT servers: %s", esp_err_to_name(err));
        mdns_query_results_free(results);
        return false;
    }

    if (results->instance_name) {
        printf("  PTR : %s\n", results->instance_name);
    }
    if (results->hostname) {
        printf("  SRV : %s.local:%u\n", results->hostname, results->port);
    }

    snprintf(mqtt_server.hostname, sizeof(mqtt_server.hostname), "%s.local",
        results->hostname);
    ESP_LOGI("MDNS", "Found MQTT server: %s", mqtt_server.hostname);

    snprintf(mqtt_server.ip, sizeof(mqtt_server.ip), "%s",
        ip4addr_ntoa(&(results->addr->addr.u_addr.ip4)));
    ESP_LOGI("MDNS", "Found MQTT server IP: %s", mqtt_server.ip);

    snprintf(mqtt_server.full_proto, sizeof(mqtt_server.full_proto), "mqtt://%s",
        mqtt_server.ip); // "mqtt://192.168.111.222"
    // printf("%s - %d", mqtt_server.full_proto, sizeof(mqtt_server.full_proto));

    mqtt_server.port = results->port;
    ESP_LOGI("MDNS", "Found MQTT server port: %d", mqtt_server.port);

    mdns_query_results_free(results);
    return true;
}

Mqtt::Mqtt(QueueHandle_t& temperature_queue, QueueHandle_t& percent_queue)
{
    // Queue from sensor
    _sensor_queue = &temperature_queue;
    _percent_queue = &percent_queue;
    // Create queue set
    _queue_set = xQueueCreateSet(2);
    xQueueAddToSet(*_sensor_queue, _queue_set);
    xQueueAddToSet(*_percent_queue, _queue_set);

    // Client initialisation
    if (_state == state_m::NOT_INITIALISED) {
        // wait for WI-FI connection and IP
        xEventGroupWaitBits(common_event_group, _wifi_got_ip, pdFALSE, pdFALSE,
            portMAX_DELAY);
        if (!find_mqtt_server(_mdns_mqtt_server)) {
            mqtt_cfg.uri = CONFIG_BROKER_URL;
            mqtt_cfg.port = CONFIG_BROKER_PORT;
        } else {
            mqtt_cfg.uri = _mdns_mqtt_server.full_proto;
            mqtt_cfg.port = _mdns_mqtt_server.port;
        }
    }
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
        // printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        // printf("DATA=%.*s\r\n", event->data_len, event->data);
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
    }
    // Publish sensor data
    SensorData_t sensor_data {};
    uint8_t percent {};

    for (;;) {
        QueueSetMemberHandle_t activate_handle = xQueueSelectFromSet(_queue_set, portMAX_DELAY);

        if (activate_handle == *_sensor_queue) {
            if (xQueueReceive(*_sensor_queue, &sensor_data, portMAX_DELAY) == pdTRUE) {
                if (sensor_data.temperature == 0.0f && sensor_data.sensor_id == 0) {
                    ESP_LOGW(TAG, "Получено нулевое значение от датчика %d",
                        sensor_data.sensor_id);
                    continue; // skip sending
                }
                char msg[10]; // buffer for message
                snprintf(msg, sizeof(msg), "%.2f", sensor_data.temperature);

                char topic[50]; // buffer for topic
                snprintf(topic, sizeof(topic), "/topic/%d", sensor_data.sensor_id);

                ESP_LOGI(TAG, "Temperature from MQTT: %s %s", msg, topic);
                esp_mqtt_client_publish(client, topic, msg, 0, 0, 0);
            } else {
                ESP_LOGE(TAG, "Failed to receive data from sensor queue");
            }

        } else if (activate_handle == *_percent_queue) {
            if (xQueueReceive(*_percent_queue, &percent, portMAX_DELAY) == pdTRUE) {

                char msg[10]; // buffer for message
                snprintf(msg, sizeof(msg), "%d", percent);

                char topic[50]; // buffer for topic
                snprintf(topic, sizeof(topic), "/topic/pecent");

                ESP_LOGI(TAG, "Percent from MQTT: %s %s", msg, topic);
                esp_mqtt_client_publish(client, topic, msg, 0, 0, 0);
            } else {
                ESP_LOGE(TAG, "Failed to receive data from percent queue");
            }
        }
    }
}

} // namespace Mqtt_NS
