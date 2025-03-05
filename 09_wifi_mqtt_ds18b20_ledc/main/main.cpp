#include "FreeRTOS.h" // IWYU pragma: keep
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "fan.h"
#include "freertos/queue.h"
#include "gpio.h" // IWYU pragma: keep
#include "mqtt.h"
#include "projdefs.h"
#include "wifi_simple.h"
#include <cstddef>
#include <cstdio>

constexpr uint8_t SENSOR_COUNT { 2 };
constexpr uint16_t STACK_TASK_SIZE { 3072 }; // 1024 * 3

// ============================ Global Variables ==============================

// Structures for sensor
typedef struct {
    OneWire::DS18B20& onewire_pin;
    // float& temp;
    uint8_t (*sensor_addr)[8];
} Temp_param;

// TODO: Make class Event Manager
EventGroupHandle_t common_event_group = xEventGroupCreate();
uint8_t _wifi_connect_bit { BIT0 };
uint8_t _wifi_disconnect_bit { BIT1 };
uint8_t _wifi_got_ip { BIT2 };
uint8_t _wifi_lost_ip { BIT3 };

QueueHandle_t temperature_queue_PWM = xQueueCreate(5, sizeof(SensorData_t));
QueueHandle_t duty_percent_queue = xQueueCreate(5, sizeof(uint8_t));
QueueHandle_t temperature_queue = xQueueCreate(5, sizeof(SensorData_t));

// ===================== FreeRTOS Tasks =======================================
TaskHandle_t wifi_connection_handle = NULL;
void wifi_connection(void* pvParameter)
{
    Wifi_NS::Wifi* wifi_obj = static_cast<Wifi_NS::Wifi*>(pvParameter);
    for (;;) {
        wifi_obj->start();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

TaskHandle_t mqtt_connection_handle = NULL;
void mqtt_connection(void* pvParameter)
{
    Mqtt_NS::Mqtt mqtt(temperature_queue, duty_percent_queue);
    for (;;) {
        mqtt.start();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Get temperature task
TaskHandle_t get_temperature_handle = NULL;
void get_temperature(void* pvParameter)
{
    Temp_param* temp_param = static_cast<Temp_param*>(pvParameter);
    SensorData_t sensor_data = {};
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
            sensor_data.sensor_id = i;
            temp_param->onewire_pin.get_temp(temp_param->sensor_addr[i],
                sensor_data.temperature);

            ESP_LOGI("DS18B20", "Temperature %d: %.2f", i, sensor_data.temperature);

            if (xQueueSend(temperature_queue, &sensor_data, portMAX_DELAY) != pdPASS) {
                ESP_LOGE("DS18B20", "Failed to send data to queue from main.cpp");
            }
            if (xQueueSend(temperature_queue_PWM, &sensor_data, portMAX_DELAY) != pdPASS) {
                ESP_LOGE("DS18B20", "Failed to send data to queue from main.cpp");
            }
        }
    }
}

// ===================== Debugging ============================================
void checkStackUsage(void* pvParameter)
{
    {
        for (;;) {
            UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(get_temperature_handle);
            // Warning for visual difference
            ESP_LOGW("StackMonitor", "High Water Mark:%u", highWaterMark);
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}
void printHeapInfo()
{
    // Print heap info
    uint32_t freeHeap = esp_get_free_heap_size();
    uint32_t freeHeapMin = esp_get_minimum_free_heap_size();
    // Warning for visual difference
    ESP_LOGW("HeapMonitor", "Free Heap Size: %u bytes", freeHeap);
    ESP_LOGW("HeapMonitor", "Minimal Heap Size: %u bytes", freeHeapMin);
}

void heapMonitor(void* pvParameter)
{
    for (;;) {
        printHeapInfo();
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// ============================================================================
// ============================= Main Program =================================
// ============================================================================

extern "C" void app_main(void)
{
    // =========================== Logging ====================================
    // Set log level in this scope
    esp_log_level_set("*", ESP_LOG_INFO);
    // esp_log_level_set("*", ESP_LOG_WARN);
    // esp_log_level_set("*", ESP_LOG_ERROR);

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

    // ======================= Tasks Looping ==================================
    xTaskCreate(&wifi_connection, "Wifi", STACK_TASK_SIZE, &wifi, 5,
        &wifi_connection_handle);

    xTaskCreate(&mqtt_connection, "Mqtt", STACK_TASK_SIZE, NULL, 5,
        &mqtt_connection_handle);

    xTaskCreate(&get_temperature, "Temperature", STACK_TASK_SIZE, &param, 5,
        &get_temperature_handle);

    // Debug tasks
    // xTaskCreate(checkStackUsage, "CheckStack", STACK_TASK_SIZE, NULL, 5, NULL);
    xTaskCreate(&heapMonitor, "HeapMonitor", STACK_TASK_SIZE, NULL, 5, NULL);

    // Fan control.
    // NOTE: Keep this after all other tasks, because it not working
    // in FreeRTOS task.
    Fan_NS::FanPWM fan_pwm(13, &temperature_queue_PWM, &duty_percent_queue);
    fan_pwm.start();

    // ======================= FreeRTOS Specific =============================
    // BUG: Without this, FreeRTOS tasks will crash
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskStartScheduler();
}
