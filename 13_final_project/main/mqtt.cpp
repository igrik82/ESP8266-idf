#include "mqtt.h"
#include "esp_err.h"
#include "esp_event.h"
#include "mdns.h"
#include "mqtt_client.h"
#include <cstddef>
#include <cstdint>

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
uint8_t Mqtt::_connection_retry { 0 };

// Constructor
Mqtt::Mqtt(QueueHandle_t& temperature_queue, QueueHandle_t& percent_queue)
    : _sensor_queue(&temperature_queue)
    , _percent_queue(&percent_queue)
    , _mdns_mqtt_server({})
{
    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
        WIFI_EVENT_STA_DISCONNECTED,
        &Mqtt::_disconnect_handler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
        &Mqtt::_connect_handler, this));
}

void Mqtt::_disconnect_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    Mqtt* self = static_cast<Mqtt*>(arg);
    if (self->client == NULL) {
        return;
    }
    self->stop(self->client);
}

void Mqtt::stop(esp_mqtt_client_handle_t client)
{
    ESP_LOGI(TAG, "Stoping mqtt client...");
    esp_mqtt_client_destroy(client);
    Mqtt::client = NULL;
    _state = state_m::NOT_INITIALISED;
    if (_queue_set) {
        ESP_LOGI(TAG, "Deleting queue set from mqtt client");
        vQueueDelete(_queue_set);
        _queue_set = nullptr;
    }
}

bool Mqtt::find_mqtt_server(MdnsMqttServer_t& mqtt_server)
{

    // Initialize mDNS
    ESP_ERROR_CHECK(mdns_init());
    ESP_LOGI(TAG_mDNS, "Querying for MQTT servers...");
    mdns_result_t* results = NULL;
    esp_err_t err = mdns_query_ptr("_mqtt", "_tcp", 3000, 20, &results);

    if (err != ESP_OK || results == NULL) {
        ESP_LOGW(TAG_mDNS, "Failed to find MQTT servers");
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
    ESP_LOGI(TAG_mDNS, "Found MQTT server: %s", mqtt_server.hostname);

    snprintf(mqtt_server.ip, sizeof(mqtt_server.ip), "%s",
        ip4addr_ntoa(&(results->addr->addr.u_addr.ip4)));
    ESP_LOGI(TAG_mDNS, "Found MQTT server IP: %s", mqtt_server.ip);

    snprintf(mqtt_server.full_proto, sizeof(mqtt_server.full_proto), "mqtt://%s",
        mqtt_server.ip); // "mqtt://192.168.111.222"

    mqtt_server.port = results->port;
    ESP_LOGI(TAG_mDNS, "Found MQTT server port: %d", mqtt_server.port);

    mdns_query_results_free(results);

    mdns_free();
    return true;
}

void Mqtt::_connect_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    Mqtt* self = static_cast<Mqtt*>(arg);
    if (self->client == NULL) {
        ESP_LOGI(TAG, "Initializing client...");
        self->init();
    }
}

// Initialisation mqtt client
void Mqtt::init(void)
{
    if (_state == state_m::INITIALISED) {
        return;
    }

    // Create queue set
    if (_queue_set != nullptr) {
        _queue_set = xQueueCreateSet(2);
        xQueueAddToSet(*_sensor_queue, _queue_set);
        xQueueAddToSet(*_percent_queue, _queue_set);
    }

    // Search for mDNS MQTT server
    while (!find_mqtt_server(_mdns_mqtt_server)) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }

    // Connect only through mDNS
    mqtt_cfg.uri = _mdns_mqtt_server.full_proto;
    mqtt_cfg.port = _mdns_mqtt_server.port;
    // mqtt_cfg.uri = CONFIG_BROKER_URL;
    // mqtt_cfg.port = CONFIG_BROKER_PORT;

    mqtt_cfg.client_id = CONFIG_CLIENT_ID;
    mqtt_cfg.username = MQTT_USER;
    mqtt_cfg.password = MQTT_PASSWORD;
    mqtt_cfg.keepalive = CONFIG_MQTT_KEEP_ALIVE;
    mqtt_cfg.protocol_ver = MQTT_PROTOCOL_V_3_1_1;
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_err_t ret = esp_mqtt_client_register_event(client, MQTT_EVENT_ANY,
        mqtt_event_handler, this);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register event handler: %s", esp_err_to_name(ret));
    }
    _state = state_m::INITIALISED;
    ESP_LOGI(TAG, "Client initialiased");

    // Call start
    start(client);
}

void Mqtt::start(esp_mqtt_client_handle_t client)
{
    if (client != nullptr and _state == state_m::INITIALISED) {
        esp_mqtt_client_start(client);
        _state = state_m::STARTED;
        ESP_LOGI(TAG, "Client started");
        // Watcher for connection
        connection_watcher(client);
    }
}

void Mqtt::connection_watcher(esp_mqtt_client_handle_t client)
{
    xEventGroupWaitBits(common_event_group, _mqtt_disconnect_bit, pdTRUE, pdFALSE,
        portMAX_DELAY);
    stop(client);
    init();
}

// Events handler
void Mqtt::mqtt_event_handler(void* handler_args, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%d",
        event_base, event_id);
    Mqtt* instance = static_cast<Mqtt*>(handler_args);
    if (instance) {
        instance->mqtt_event_handler_cb(
            static_cast<esp_mqtt_event_handle_t>(event_data));
    }
}

// Evens callback
esp_err_t Mqtt::mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    ESP_LOGI(TAG, "in event %d", event->event_id);
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connected to MQTT broker at %s:%d", mqtt_cfg.uri,
            mqtt_cfg.port);
        ESP_LOGI(TAG, "Login - \"%s\" password - \"%s\"", mqtt_cfg.username,
            mqtt_cfg.password);
        _state = state_m::CONNECTED;
        Mqtt::_connection_retry = 0;
        break;
    case MQTT_EVENT_DISCONNECTED:

        // Retry connection 20 times about 3 minutes
        if (Mqtt::_connection_retry > 20) {
            ESP_LOGW(
                TAG,
                "Client was not able to connect to MQTT broker. Deinitializing...");
            Mqtt::_connection_retry = 0;
            xEventGroupSetBits(common_event_group, _mqtt_disconnect_bit);
            break;
        }
        ESP_LOGI(TAG, "Trying to connect MQTT broker. Try # %d",
            Mqtt::_connection_retry + 1);
        if (_state == state_m::CONNECTED) {
            ESP_LOGI(TAG, "Disconnected from MQTT broker at %s:%d", mqtt_cfg.uri,
                mqtt_cfg.port);
            _state = state_m::DISCONNECTED;
        }
        Mqtt::_connection_retry++;
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
// Start MQTT client
void Mqtt::publish()
{
    // Publish sensor data
    SensorData_t sensor_data {};
    uint8_t percent {};

    const std::string device = R"({
  "device": {
    "name": "HDD Station",
    "model": "HDD Station",
    "ids": "DockHDD24D7EB118208",
    "mf": "Игорь Смоляков",
    "sw": "2.00",
    "hw": "1.01",
    "cu": "http://192.168.88.13"
  },)";

    // Temperature left HDD
    const std::string topic_left = R"(homeassistant/sensor/HDDdock_temp_left/config)";
    const std::string msg_left = R"(
  "name": "Left HDD",
  "deve_cla": "temperature",
  "stat_t": "homeassistant/sensor/HDDdock/temp_0/state",
  "uniq_id": "DockHDD_temp_left",
  "icon": "mdi:harddisk",
  "unit_of_meas": "°C"
})";
    esp_mqtt_client_publish(client, topic_left.c_str(),
        (device + msg_left).c_str(), 0, 1, 1);

    // Temperature right HDD
    const std::string topic_right = R"(homeassistant/sensor/HDDdock_temp_right/config)";
    const std::string msg_right = R"(
  "name": "Right HDD",
  "deve_cla": "temperature",
  "stat_t": "homeassistant/sensor/HDDdock/temp_1/state",
  "uniq_id": "DockHDD_temp_right",
  "icon": "mdi:harddisk",
  "unit_of_meas": "°C"
 })";
    esp_mqtt_client_publish(client, topic_right.c_str(),
        (device + msg_right).c_str(), 0, 1, 1);

    // Fan power
    const std::string topic_fan = R"(homeassistant/sensor/HDDdock_fan/config)";
    const std::string msg_fan = R"(
    "name": "Fan HDD",
  "deve_cla": "power_factor",
  "stat_t": "homeassistant/sensor/HDDdock/fan/state",
  "uniq_id": "DockHDD_fan",
  "icon": "mdi:fan",
  "unit_of_meas": "%"
 })";
    esp_mqtt_client_publish(client, topic_fan.c_str(), (device + msg_fan).c_str(),
        0, 1, 1);

    for (;;) {
        QueueSetMemberHandle_t activate_handle = xQueueSelectFromSet(_queue_set, portMAX_DELAY);
        if (_state != state_m::INITIALISED) {
            return;
        }

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
                snprintf(topic, sizeof(topic),
                    "homeassistant/sensor/HDDdock/temp_%d/state",
                    sensor_data.sensor_id);

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
                snprintf(topic, sizeof(topic),
                    "homeassistant/sensor/HDDdock/fan/state");

                ESP_LOGI(TAG, "Percent from MQTT: %s %s", msg, topic);
                esp_mqtt_client_publish(client, topic, msg, 0, 0, 0);
            } else {
                ESP_LOGE(TAG, "Failed to receive data from percent queue");
            }
        }
    }
}

} // namespace Mqtt_NS
