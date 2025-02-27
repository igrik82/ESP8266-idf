#include "FreeRTOS.h" // IWYU pragma: keep
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "gpio.h" // IWYU pragma: keep
#include "mqtt.h"
#include "projdefs.h"
#include "wifi_simple.h"
#include <cstddef>
#include <cstdio>

#define SENSOR_COUNT 2

// ============================ Global variables ==============================

// Structures for sensor
typedef struct {
    OneWire::DS18B20& onewire_pin;
    // float& temp;
    uint8_t (*sensor_addr)[8];
} Temp_param;

EventGroupHandle_t common_event_group = xEventGroupCreate();
QueueHandle_t temperature_queue = xQueueCreate(5, sizeof(SensorData_t));

uint8_t _wifi_connect_bit { BIT0 };
uint8_t _wifi_disconnect_bit { BIT1 };
uint8_t _wifi_got_ip { BIT2 };
uint8_t _wifi_lost_ip { BIT3 };

// ===================== Functions ===========================================
TaskHandle_t wifi_connection_handle = NULL;
void wifi_connection(void* pvParameter)
{
    Wifi_NS::Wifi* wifi_obj = (Wifi_NS::Wifi*)pvParameter;
    for (;;) {
        wifi_obj->start();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

TaskHandle_t mqtt_connection_handle = NULL;
void mqtt_connection(void* pvParameter)
{
    for (;;) {
        Mqtt_NS::Mqtt mqtt(temperature_queue);
        mqtt.start();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Get temperature task
TaskHandle_t get_temperature_handle = NULL;

void get_temperature(void* pvParameter)
{
    Temp_param* temp_param = (Temp_param*)pvParameter;
    SensorData_t sensor_data = {};
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
            sensor_data.sensor_id = i;
            temp_param->onewire_pin.get_temp(temp_param->sensor_addr[i],
                sensor_data.temperature);

            printf("Temperature %d: %.2f\n", i, sensor_data.temperature);

            xQueueSend(temperature_queue, &sensor_data, portMAX_DELAY);
        }
    }
}

// ===================== Debugging ============================================
void checkStackUsage(void* pvParameter)
{
    {
        for (;;) {
            UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(wifi_connection_handle);
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
    // =========================== Logging ====================================
    // Set log level in this scope
    // [[maybe_unused]] const char* TAG = "Wi-Fi";
    // esp_log_level_set(TAG, ESP_LOG_INFO);
    // esp_log_level_set("*", ESP_LOG_VERBOSE);
    esp_log_level_set("*", ESP_LOG_ERROR);

    // ========================= Initialization ===============================

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());

    // Wifi initialization
    Wifi_NS::Wifi wifi;

    // DS18B20 initialization
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

    Temp_param param = { onewire_pin, ds18b20_address };

    // ======================= Tasks looping ==================================
    xTaskCreate(&wifi_connection, "Wifi", 1024 * 2, &wifi, 5,
        &wifi_connection_handle);

    xTaskCreate(&mqtt_connection, "Mqtt", 1024 * 2, NULL, 5,
        &mqtt_connection_handle);

    xTaskCreate(&get_temperature, "Temperature", 1024 * 2, &param, 5,
        &get_temperature_handle);

    // Debug tasks
    // xTaskCreate(checkStackUsage, "CheckStack", 1024 * 2, NULL, 5, NULL);
    xTaskCreate(&heapMonitor, "HeapMonitor", 1024 * 2, NULL, 5, NULL);

    // ======================= FreeRTOS specific =============================
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskStartScheduler();
}
