#include "esp_err.h" // IWYU pragma: keep
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"
#include "gpio.h"
// #include <cstdint>
// #include <cstdio>

#define pdMILISECONDS pdMS_TO_TICKS

extern "C" void app_main(void)
{
    //============================ Logging ====================================
    // Set log level in this scope
    [[maybe_unused]] const char* TAG = "DS18B20";
    // esp_log_level_set(TAG, ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_ERROR);

    //========================== Initialization ===============================

    OneWire::DS18B20 onewire_pin { GPIO_NUM_12 };
    // Uncomment this to read ROM address of DS18B20, one sensor at a time
    // onewire_pin.readROM();

    // Array of DS18B20 addresses
    uint8_t ds18b20_address[][8] = {
        // Left temperature sensor
        { 0x28, 0xf5, 0x48, 0x16, 0x00, 0x00, 0x00, 0x61 },
        // Right temperature sensor
        { 0x28, 0x1c, 0xc1, 0x11, 0x00, 0x00, 0x00, 0x60 },
        // ...
    };

    uint8_t array_size = sizeof(ds18b20_address) / sizeof(*ds18b20_address);
    float temperature {};

    //============================= Looping ===================================

    for (;;) {

        vTaskDelay(pdMILISECONDS(1000));
        // ======================== Read temperature ==========================
        for (uint8_t i = 0; i < array_size; i++) {
            onewire_pin.get_temp(ds18b20_address[i], temperature);
            printf("Temperature %d: %d\n", i + 1, (uint8_t)temperature);
            vTaskDelay(pdMILISECONDS(10));
        }
        printf("\n");
    }
}
