#pragma once

#include "http_web_page.h"
#include <cstddef>
#include <sys/param.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#include <esp_http_server.h>

namespace Http_NS {
class HttpServer {
private:
    httpd_handle_t _server = nullptr;
    static void _connect_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data);
    static void _disconnect_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data);

    static httpd_handle_t start_webserver(void);

public:
    HttpServer(void);
    ~HttpServer(void) = default;
    constexpr static const char* TAG = "HTTPServer";
};
} // Namespace Http_NS
