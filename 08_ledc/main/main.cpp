#include "FreeRTOS.h" // IWYU pragma: keep
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "fan.h"
#include "projdefs.h"
#include <cstdio>
#include <stdlib.h>

QueueHandle_t sensor_queue = xQueueCreate(5, sizeof(SensorData_t));
QueueHandle_t duty_percent_queue = xQueueCreate(5, sizeof(uint8_t));

// Test task for send data
TaskHandle_t send_data_handle = NULL;
void send_data(void* pvParameter)
{
    const uint8_t queue_size { 6 };
    SensorData_t sensor_data[queue_size];
    for (;;) {

        for (uint8_t i = 0; i < queue_size; i++) {
            sensor_data[i].sensor_id = 0;
            sensor_data[i].temperature = 20.0f + (rand() % 26); // 20-46°C
        }

        for (uint8_t i = 0; i < queue_size; i++) {
            ESP_LOGI(Fan_NS::FanPWM ::TAG, "Sensor %d temp %f",
                sensor_data[i].sensor_id, sensor_data[i].temperature);
            xQueueSend(sensor_queue, &sensor_data[i], portMAX_DELAY);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

TaskHandle_t receive_percent_handle = NULL;
void receive_percent(void* pvParameter)
{
    uint8_t percent { 0 };
    for (;;) {
        xQueueReceive(duty_percent_queue, &percent, portMAX_DELAY);
        ESP_LOGI(Fan_NS::FanPWM ::TAG, "Fan speed %d %%", percent);
    }
}

// ===================== Debugging ============================================
// void checkStackUsage(void* pvParameter)
// {
//     {
//         for (;;) {
//             UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark();
//             printf("High Water Mark:%u\n", highWaterMark);
//             vTaskDelay(pdMS_TO_TICKS(5000));
//         }
//     }
// }
void printHeapInfo()
{
    // Print heap info
    uint32_t freeHeap = esp_get_free_heap_size();
    uint32_t freeHeapMin = esp_get_minimum_free_heap_size();
    ESP_LOGI("HeapMonitor", "Free Heap Size: %u bytes", freeHeap);
    ESP_LOGI("HeapMonitor", "Minimal Heap Size: %u bytes", freeHeapMin);
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
    esp_log_level_set("*", ESP_LOG_INFO);
    // esp_log_level_set("*", ESP_LOG_WARN);

    // ========================= Initialization ===============================
    // xTaskCreate(heapMonitor, "HeapMonitor", 2048, NULL, 1, NULL);
    // xTaskCreate(checkStackUsage, "CheckStackUsage", 2048, NULL, 1, NULL);

    // ======================= Tasks looping ==================================
    // Debug tasks
    // xTaskCreate(checkStackUsage, "CheckStack", 1024 * 2, NULL, 5, NULL);
    xTaskCreate(&heapMonitor, "HeapMonitor", 1024 * 2, NULL, 5, NULL);
    xTaskCreate(&receive_percent, "ReceivePercent", 1024 * 2, NULL, 5,
        &receive_percent_handle);

    xTaskCreate(&send_data, "SendData", 1024 * 2, NULL, 5, &send_data_handle);
    Fan_NS::FanPWM fan_pwm(2, &sensor_queue, &duty_percent_queue);
    fan_pwm.start();

    // xTaskCreate(fan_control, "FanControl", 1024 * 4, &fan_pwm, 5, NULL);

    // ======================= FreeRTOS specific =============================
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskStartScheduler();
}
