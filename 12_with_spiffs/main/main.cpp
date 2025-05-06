#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "http_pages.h"
#include <cstdint>
#include <cstring>
#include <sys/stat.h>

const char* TAG = "SPIFFS";

void write_to_flash(const uint8_t* content, size_t size,
    const char* file_name)
{
    // Write a file
    FILE* fd = fopen(file_name, "w");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to create file (errno: %d)", errno);
        return;
    }

    fwrite(content, 1, size, fd);
    fclose(fd);

    // Writing file
    struct stat st;
    if (stat(file_name, &st) == 0) {
        ESP_LOGI(TAG, "File %s verified. Size: %ld (expected %zu)", file_name,
            st.st_size, size);
        if (st.st_size != size) {
            ESP_LOGE(TAG, "FILE CORRUPTION DETECTED!");
        }
    }
}
// ============================================================================
// ============================= Main Program
// =================================
// ============================================================================

extern "C" void app_main(void)
{
    // =========================== Logging ====================================
    // Set log level in this scope
    esp_log_level_set("*", ESP_LOG_INFO);
    // esp_log_level_set("*", ESP_LOG_WARN);
    // esp_log_level_set("*", ESP_LOG_ERROR);

    // ========================= Initialization ===============================
    // Formatting
    ESP_LOGI(TAG, "Formatting SPIFFS...");
    esp_spiffs_format("storage");

    // Inalizing and mounting SPIFFS
    esp_vfs_spiffs_conf_t spiffs_config;
    spiffs_config.base_path = "/spiffs";
    spiffs_config.partition_label = "storage";
    spiffs_config.max_files = 5;
    spiffs_config.format_if_mount_failed = true;
    esp_err_t ret = esp_vfs_spiffs_register(&spiffs_config);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI("SPIFFS", "Finished mounting SPIFFS");

    // Check size
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(spiffs_config.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)",
            esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %zu, used: %zu", total, used);
    }

    // Write files to SPIFFS
    write_to_flash(index_html, size_index_html, "/spiffs/index.html");
    write_to_flash(favicon_cont, size_favicon_cont, "/spiffs/favicon.ico");

    // Check size
    total = 0, used = 0;
    ret = esp_spiffs_info(spiffs_config.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)",
            esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %zu, used: %zu", total, used);
        ESP_LOGI(TAG, "All files written to SPIFFS successfully!");
    }
    // ======================= FreeRTOS Specific =============================
    // BUG: Without this, FreeRTOS tasks will crash
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskStartScheduler();
}
