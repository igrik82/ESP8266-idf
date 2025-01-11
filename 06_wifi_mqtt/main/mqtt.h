#pragma once

#include "esp_log.h" // IWYU pragma: keep
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "nvs_flash.h" // IWYU pragma: keep
#include <cstdint>
namespace Mqtt_NS {

class Mqtt {
private:
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

public:
    Mqtt(void);
    ~Mqtt(void) = default;
    void start(void);
    esp_err_t stop(void);
    constexpr static const char* TAG = "MQTT";
};

} // namespace Mqtt_NS
