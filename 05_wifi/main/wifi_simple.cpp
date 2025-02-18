#include "wifi_simple.h"

namespace Wifi_NS {

// Static variables
Wifi::state_w Wifi::_state { state_w::NOT_INITIALISED };
wifi_init_config_t Wifi::_wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
wifi_config_t Wifi::wifi_config {};
EventGroupHandle_t Wifi::_wifi_event_group = xEventGroupCreate();

Wifi::Wifi(void)
{

    tcpip_adapter_init();
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_wifi_init(&_wifi_init_config));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
        &_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID,
        &_event_handler, NULL));
    memcpy(wifi_config.sta.ssid, WIFI_SSID, strlen(WIFI_SSID));
    memcpy(wifi_config.sta.password, WIFI_PASSWORD, strlen(WIFI_PASSWORD));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
#if CONFIG_CUSTOM_MAC
    esp_wifi_set_mac(WIFI_IF_STA, _mac_addr);
#endif
    esp_wifi_set_ps(POWER_MODE);
};

// TODO: Make deinitialise wifi
// Wifi_NS::Wifi::~Wifi(void) { };

esp_err_t Wifi::_led_on(void)
{
    gpio_set_direction(_led, GPIO_MODE_OUTPUT);
    return gpio_set_level(_led, 0);
}

// TODO: Make led blink
// void Wifi::_led_blink(void) {
//   gpio_set_direction(_led, GPIO_MODE_OUTPUT);
//   for (;;) {
//     gpio_set_level(_led, 0);
//     vTaskDelay(pdMS_TO_TICKS(500));
//     gpio_set_level(_led, 1);
//     vTaskDelay(pdMS_TO_TICKS(500));
//   }
// }

esp_err_t Wifi::_led_off(void)
{
    gpio_set_direction(_led, GPIO_MODE_OUTPUT);
    return gpio_set_level(_led, 1);
}

void Wifi::_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    if (WIFI_EVENT == event_base) {
        return _wifi_event_handler(arg, event_base, event_id, event_data);
    } else if (IP_EVENT == event_base) {
        return _ip_event_handler(arg, event_base, event_id, event_data);
    } else {
        ESP_LOGE(TAG, "Unexpected event %s", event_base);
    }
}

void Wifi::_wifi_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    switch (event_id) {

    case WIFI_EVENT_STA_START:
        _state = state_w::READY_TO_CONNECT;
        ESP_LOGI(TAG, "Connecting to AP...");
        break;

    case WIFI_EVENT_STA_CONNECTED:
        _state = state_w::WAITING_FOR_IP;
        xEventGroupSetBits(_wifi_event_group, _wifi_connect_bit);
        ESP_LOGI(TAG, "Connected to AP. Waiting for IP address");
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
        _state = state_w::DISCONNECTED;
        xEventGroupSetBits(_wifi_event_group, _wifi_disconnect_bit);
        break;

    default:
        _state = state_w::ERROR;
        ESP_LOGE(TAG, "Unexpected event %d", event_id);
        break;
    }
}

void Wifi::_ip_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    switch (event_id) {

    case IP_EVENT_STA_GOT_IP:
        _state = state_w::CONNECTED;
        ESP_LOGI(TAG, "Got IP Address");
        xEventGroupSetBits(_wifi_event_group, _wifi_got_ip);
        break;

    case IP_EVENT_STA_LOST_IP:
        _state = state_w::DISCONNECTED;
        ESP_LOGI(TAG, "Lost IP Address");
        xEventGroupSetBits(_wifi_event_group, _wifi_lost_ip);
        break;

    default:
        _state = state_w::ERROR;
        ESP_LOGE(TAG, "Unexpected event %d", event_id);
        break;
    }
}

esp_err_t Wifi::start(void)
{
    esp_err_t status { ESP_OK };
    if (_state == state_w::NOT_INITIALISED) {

        ESP_ERROR_CHECK(esp_wifi_start());
        esp_wifi_connect();

        _state = state_w::INITIALISED;
    }

    for (;;) {
        EventBits_t bits = xEventGroupWaitBits(
            _wifi_event_group, _wifi_connect_bit | _wifi_disconnect_bit, pdFALSE,
            pdFALSE, portMAX_DELAY);
        if (bits & _wifi_connect_bit) {
            ESP_LOGI(TAG, "Connected to AP successfully! LED on");
            _state = state_w::CONNECTED;
            xEventGroupClearBits(_wifi_event_group, _wifi_connect_bit);
            _led_on();
        } else if (bits & _wifi_disconnect_bit) {
            ESP_LOGI(TAG, "Disconnected from AP. LED off");
            _state = state_w::DISCONNECTED;
            xEventGroupClearBits(_wifi_event_group, _wifi_disconnect_bit);
            _led_off();
            // _led_blink();
            esp_wifi_connect();
        }
    }

    return status;
}
} // namespace Wifi_NS
