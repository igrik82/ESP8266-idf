#pragma once

#include "esp_event.h"
#include "esp_log.h" // IWYU pragma: keep
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "nvs_flash.h" // IWYU pragma: keep
#include "secrets.h" // IWYU pragma: keep
#include <cstdint>

typedef struct {
    uint8_t sensor_id; // ID sensor
    float temperature; // Temperature
} SensorData_t;

namespace Mqtt_NS {

class Mqtt {
private:
    enum class state_m {
        NOT_INITIALISED,
        INITIALISED,
        // READY_TO_CONNECT,
        // CONNECTING,
        // WAITING_FOR_IP,
        // CONNECTED,
        // DISCONNECTED,
        // ERROR
    };
    static state_m _state;
    esp_mqtt_client_handle_t client;
    static esp_mqtt_client_config_t mqtt_cfg;
    /*
        wifi 0 bit - connected
        wifi 1 bit - disconnected
        wifi 2 bit - got ip
        wifi 3 bit - lost ip
        wifi 4 bit - reserved
        mqtt 5 bit - connected
        mqtt 6 bit - disconnected
        mqtt 7 bit - subscribed
        mqtt 8 bit - unsubscribed
        mqtt 9 bit - reserved
    */
    const uint8_t _mqtt_connect_bit { BIT5 };
    const uint8_t _mqtt_disconnect_bit { BIT6 };
    const uint8_t _mqtt_subscribed { BIT7 };
    const uint16_t _mqtt_unsubscribed { BIT8 };

    static void mqtt_event_handler(void* handler_args, esp_event_base_t base,
        int32_t event_id, void* event_data);
    static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event);

    QueueHandle_t* _sensor_queue;

public:
    Mqtt(QueueHandle_t& temperature_queue);
    ~Mqtt(void) = default;
    void start(void);
    constexpr static const char* TAG = "MQTT";
};

} // namespace Mqtt_NS
