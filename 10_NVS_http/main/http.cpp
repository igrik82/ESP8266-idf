#include "http.h"
#include "esp_http_server.h"

namespace Http_NS {

httpd_handle_t* HttpServer::_server = NULL;

HttpServer::HttpServer(void)
{
    // initialize and mounting SPIFFS
    esp_vfs_spiffs_conf_t spiffs_config;
    spiffs_config.base_path = "/spiffs";
    spiffs_config.partition_label = "storage";
    spiffs_config.max_files = 5;
    spiffs_config.format_if_mount_failed = true;
    esp_err_t ret = esp_vfs_spiffs_register(&spiffs_config);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG_SPIFF, "Failed to mount or format filesystem");
        } else {
            ESP_LOGE(TAG_SPIFF, "Failed to initialize SPIFFS (%s)",
                esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI("SPIFFS", "Finished mounting SPIFFS");

    // get SPIFFS partition information
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(spiffs_config.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SPIFF, "Failed to get SPIFFS partition information (%s)",
            esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG_SPIFF, "Partition size: total: %zu, used: %zu", total, used);
    }

    // Register event handler for starting http server
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
        &_connect_handler, &_server));
}

void HttpServer::_connect_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    httpd_handle_t* _server = static_cast<httpd_handle_t*>(arg);
    if (*_server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *_server = HttpServer::start_webserver();
    }
}

/* An HTTP GET handler */
static esp_err_t root_get_handler(httpd_req_t* req)
{
    char html_file[] = "/spiffs/favicon.ico";

    // Check if destination file exists before renaming
    ESP_LOGI(HttpServer::TAG_SPIFF, "Trying to open %s", html_file);
    FILE* fd = fopen(html_file, "r");
    if (!fd) {
        ESP_LOGE(HttpServer::TAG_SPIFF, "Failed to open %s", html_file);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    ESP_LOGI("SPIFFS", "File %s opened successfully", html_file);

    char buffer[512];
    size_t bytes_read;
    struct stat file_stat;

    if (stat(html_file, &file_stat) != 0) {
        ESP_LOGE(HttpServer::TAG, "Failed to get file stats");
        fclose(fd);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ESP_LOGI(HttpServer::TAG, "Sending file: %s (%ld bytes)...", html_file,
        file_stat.st_size);

    // Set the content type header
    httpd_resp_set_type(req, "image/x-icon");

    // Send the file
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fd)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK) {
            ESP_LOGE(HttpServer::TAG, "File sending failed!");
            fclose(fd);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
    }
    fclose(fd);

    // Always finish sending the response
    httpd_resp_send_chunk(req, NULL, 0);

    ESP_LOGI(HttpServer::TAG, "File sending complete");
    return ESP_OK;
}

httpd_uri_t root_dir = { .uri = "/favicon.ico",
    .method = HTTP_GET,
    .handler = root_get_handler,
    .user_ctx = NULL };

httpd_handle_t HttpServer::start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {

        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root_dir);
        // httpd_register_uri_handler(server, &echo);
        // httpd_register_uri_handler(server, &ctrl);
        return server;
    }

    ESP_LOGE(TAG, "Error starting server!");
    return NULL;
}

} // namespace Http_NS
