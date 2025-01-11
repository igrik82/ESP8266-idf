#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "mqtt.h"
#include "projdefs.h"
#include "wifi_simple.h"
#include <cstddef>
#include <cstdio>

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
    vTaskDelay(pdMS_TO_TICKS(5000));
    Mqtt_NS::Mqtt* mqtt_obj = (Mqtt_NS::Mqtt*)pvParameter;
    for (;;) {
        mqtt_obj->start();
        vTaskDelay(pdMS_TO_TICKS(1000));
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

// ============================ Global variables ==============================
EventGroupHandle_t common_event_group = xEventGroupCreate();

// ============================================================================
// ============================= Main cycle ===================================
// ============================================================================

extern "C" void app_main(void)
{
    //============================ Logging ====================================
    // Set log level in this scope
    // [[maybe_unused]] const char* TAG = "Wi-Fi";
    // esp_log_level_set(TAG, ESP_LOG_INFO);
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    // esp_log_level_set(TAG, ESP_LOG_ERROR);

    //========================== Initialization ===============================

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());

    Wifi_NS::Wifi wifi;
    Mqtt_NS::Mqtt mqtt;

    //======================== Tasks looping ==================================
    xTaskCreate(wifi_connection, "Wifi", 1536, &wifi, 5, &wifi_connection_handle);

    // xTaskCreate(mqtt_connection, "Mqtt", 1536, &mqtt, 5,
    // &mqtt_connection_handle);

    // // Debug tasks
    // xTaskCreate(checkStackUsage, "CheckStack", 1024, NULL, 1,
    //     &mqtt_connection_handle);
    xTaskCreate(heapMonitor, "HeapMonitor", 1024, NULL, 5, NULL);

    // ======================== FreeRTOS specific =============================
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskStartScheduler();
}
