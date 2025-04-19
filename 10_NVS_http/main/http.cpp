#include "http.h"
#include "http_web_page.h"

namespace Http_NS {

HttpServer::HttpServer(void)
{
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
        &_connect_handler, &_server));
}

void HttpServer::_connect_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*)arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = HttpServer::start_webserver();
    }
}

/* An HTTP GET handler */
esp_err_t root_get_handler(httpd_req_t* req)
{
    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*)req->user_ctx;
    httpd_resp_send(req, resp_str, strlen(resp_str));

    // /* After sending the HTTP response the old HTTP request
    //  * headers are lost. Check if HTTP request headers can be read now. */
    // if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
    //     ESP_LOGI(TAG, "Request headers lost");
    // }
    return ESP_OK;
}

httpd_uri_t root_dir = { .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = const_cast<char*>(web_page.c_str()) };

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
