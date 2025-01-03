#include "esp_err.h" // IWYU pragma: keep
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"
#include "gpio.h"
#include <cstdint>
#include <esp_system.h>

#define SENSOR_COUNT 2

// ===================== Functions ===========================================
// Get temperature task
TaskHandle_t get_temperature_handle = NULL;

typedef struct {
    OneWire::DS18B20& onewire_pin;
    float& temp;
    uint8_t (*sensor_addr)[8];
} Temp_param;

void get_temperature(void* pvParameter)
{
    Temp_param* temp_param = (Temp_param*)pvParameter;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
            vTaskDelay(pdMS_TO_TICKS(10));
            temp_param->onewire_pin.get_temp(temp_param->sensor_addr[i],
                temp_param->temp);
            printf("Temperature %d: %d\n", i + 1, (uint8_t)temp_param->temp);
        }
        printf("\n");
    }
}

// ===================== Debugging ============================================
void checkStackUsage(void* pvParameter)
{
    {
        for (;;) {
            UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(get_temperature_handle);
            printf("High Water Mark:%u\n", highWaterMark);
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}
void printHeapInfo()
{
    // Print heap info
    uint32_t freeHeap = esp_get_free_heap_size();
    uint32_t freeHeapMin = esp_get_minimum_free_heap_size();
    printf("Free Heap Size: %u bytes\n", freeHeap);
    printf("Minimal Heap Size: %u bytes\n", freeHeapMin);
}

void heapMonitor(void* pvParameter)
{
    while (1) {
        printHeapInfo();
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// ============================================================================
// ============================= Main cycle ===================================
// ============================================================================

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
    uint8_t ds18b20_address[SENSOR_COUNT][8] = {
        // Left temperature sensor
        { 0x28, 0xf5, 0x48, 0x16, 0x00, 0x00, 0x00, 0x61 },
        // Right temperature sensor
        { 0x28, 0x1c, 0xc1, 0x11, 0x00, 0x00, 0x00, 0x60 },
        // ...
    };

    float temperature {};
    Temp_param param = { onewire_pin, temperature, ds18b20_address };

    //======================== Tasks looping ==================================
    xTaskCreate(get_temperature, "Temperature", 1536, &param, 5,
        &get_temperature_handle);

    // Debug tasks
    // xTaskCreate(checkStackUsage, "CheckStack", 1024, NULL, 5,
    //     &get_temperature_handle);
    // xTaskCreate(heapMonitor, "HeapMonitor", 1024, NULL, 5, NULL);

    // ======================== FreeRTOS specific =============================
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskStartScheduler();
}
