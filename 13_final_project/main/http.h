#pragma once

#include "esp_err.h" // IWYU pragma: keep
#include "esp_event.h" // IWYU pragma: keep
#include "esp_http_server.h"
#include "esp_log.h" // IWYU pragma: keep
#include "esp_spiffs.h" // IWYU pragma: keep
#include "freertos/task.h"
#include <cstddef> // IWYU pragma: keep
#include <esp_http_server.h>
#include <sys/param.h>
#include <sys/stat.h>

namespace Http_NS {
class HttpServer {
private:
    static void _connect_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data);
    static void _disconnect_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data);

    static httpd_handle_t start_webserver(void);
    static httpd_handle_t stop_webserver(httpd_handle_t server);

public:
    HttpServer(void);
    ~HttpServer(void) = default;
    static httpd_handle_t _server;
    constexpr static const char* TAG = "HTTPServer";
    constexpr static const char* TAG_SPIFF = "SPIFFS";
};
} // Namespace Http_NS
