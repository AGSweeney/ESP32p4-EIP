#include "webui.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "webui_api.h"

// Forward declarations for HTML content functions
const char *webui_get_index_html(void);
const char *webui_get_status_html(void);
const char *webui_get_ethernetip_html(void);
const char *webui_get_input_assembly_html(void);
const char *webui_get_ota_html(void);

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

static esp_err_t ethernetip_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, webui_get_ethernetip_html(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t inputassembly_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, webui_get_input_assembly_html(), HTTPD_RESP_USE_STRLEN);
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
    .uri       = "/vl53l1x",
    .method    = HTTP_GET,
    .handler   = status_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t ethernetip_uri = {
    .uri       = "/outputassembly",
    .method    = HTTP_GET,
    .handler   = ethernetip_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t inputassembly_uri = {
    .uri       = "/inputassembly",
    .method    = HTTP_GET,
    .handler   = inputassembly_handler,
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
    config.max_uri_handlers = 25; // Increased to accommodate all API endpoints (was 20, need more)
    config.max_open_sockets = 7;
    config.stack_size = 16384; // Increased for large file uploads
    config.task_priority = 5;
    config.core_id = 1; // Run on Core 1 with sensor task
    config.max_req_hdr_len = 1024; // Increased for multipart headers

    ESP_LOGI(TAG, "Starting HTTP server on port %d", config.server_port);
    
    if (httpd_start(&server_handle, &config) == ESP_OK) {
        ESP_LOGI(TAG, "HTTP server started");
        
        // Register URI handlers
        httpd_register_uri_handler(server_handle, &root_uri);
        httpd_register_uri_handler(server_handle, &status_uri);
        httpd_register_uri_handler(server_handle, &ethernetip_uri);
        httpd_register_uri_handler(server_handle, &inputassembly_uri);
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

