#include "webui.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "webui_api.h"

static const char *TAG = "webui";
static httpd_handle_t server_handle = NULL;

static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, webui_get_index_html(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t status_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, webui_get_status_html(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t ota_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, webui_get_ota_html(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t root_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t status_uri = {
    .uri       = "/status",
    .method    = HTTP_GET,
    .handler   = status_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t ota_uri = {
    .uri       = "/ota",
    .method    = HTTP_GET,
    .handler   = ota_handler,
    .user_ctx  = NULL
};

bool webui_init(void)
{
    if (server_handle != NULL) {
        ESP_LOGW(TAG, "Web UI server already initialized");
        return true;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 20;
    config.max_open_sockets = 7;
    config.stack_size = 8192;
    config.task_priority = 5;
    config.core_id = 1; // Run on Core 1 with sensor task

    ESP_LOGI(TAG, "Starting HTTP server on port %d", config.server_port);
    
    if (httpd_start(&server_handle, &config) == ESP_OK) {
        ESP_LOGI(TAG, "HTTP server started");
        
        // Register URI handlers
        httpd_register_uri_handler(server_handle, &root_uri);
        httpd_register_uri_handler(server_handle, &status_uri);
        httpd_register_uri_handler(server_handle, &ota_uri);
        
        // Register API handlers
        webui_register_api_handlers(server_handle);
        
        return true;
    }
    
    ESP_LOGE(TAG, "Failed to start HTTP server");
    return false;
}

void webui_stop(void)
{
    if (server_handle != NULL) {
        httpd_stop(server_handle);
        server_handle = NULL;
        ESP_LOGI(TAG, "HTTP server stopped");
    }
}

