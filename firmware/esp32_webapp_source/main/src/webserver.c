#include <stdio.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_http_server.h"

#include "cJSON.h"

#include "util.h"
#include "flash_nvm.h"

static const char *TAG = "SERVER";

static httpd_handle_t server = NULL;

extern struct channel_info_t obj[8];

extern int pos_ch[8];

static esp_err_t on_preview_post(httpd_req_t *req)
{
    char buffer[600];
    ESP_LOGI(TAG, "URL: %s", req->uri);
    httpd_req_recv(req, buffer, req->content_len);

    ESP_LOGI(TAG, "channel: %s", buffer);
    int index = buffer[2] - '0';
    cJSON *js;
    js = cJSON_CreateObject();
    cJSON_AddItemToObject(js, "id", cJSON_CreateNumber(index));
    cJSON_AddItemToObject(js, "name", cJSON_CreateString(obj[index].name));
    cJSON_AddItemToObject(js, "pos",cJSON_CreateString(obj[index].pos));
    cJSON_AddItemToObject(js, "unit",cJSON_CreateString(unit_id_str(obj[index].unit)));
    cJSON_AddItemToObject(js, "min", cJSON_CreateNumber(obj[index].min));
    cJSON_AddItemToObject(js, "max", cJSON_CreateNumber(obj[index].max));
    
    sprintf(buffer, "CH%d", index);
    cJSON_AddItemToObject(js, "channel", cJSON_CreateString(buffer));
    char *s = cJSON_Print(js);
    httpd_resp_sendstr(req, s);
    httpd_resp_send(req, NULL, 0);
    ESP_LOGI(TAG, "%s\n", s);
    free(s);
    cJSON_Delete(js);
    return ESP_OK;
}

static esp_err_t on_save_post(httpd_req_t *req)
{
    char buffer[600];

    ESP_LOGI(TAG, "URL: %s", req->uri);
    httpd_req_recv(req, buffer, req->content_len);
    ESP_LOGI(TAG, "%s", buffer);
    cJSON *payload = cJSON_Parse(buffer);

    cJSON *iter = cJSON_GetObjectItem(payload, "channel");
    ESP_LOGI(TAG, "channel: %s", iter->valuestring);
    int index = iter->valuestring[2] - '0'; // -1 because channel start from 1 but array start from 0

    iter = cJSON_GetObjectItem(payload, "name");
    ESP_LOGI(TAG, "name: %s", iter->valuestring);
    if (strlen(iter->valuestring) >= 100)
    {
        ESP_LOGE(TAG, "Invalid channel name");
        strcpy(obj[index].name, "Invalid");
    }
    else
        strcpy(obj[index].name, iter->valuestring);

    iter = cJSON_GetObjectItem(payload, "pos");
    ESP_LOGI(TAG, "pos: %s", iter->valuestring);
    if (strlen(iter->valuestring) >= 5)
    {
        ESP_LOGE(TAG, "Invalid channel name");
        strcpy(obj[index].name, "Invalid");
    }
    else 
    {
        strcpy(obj[index].pos, iter->valuestring);
        pos_ch[obj[index].pos[0] - 'A'] = index;
    }
    iter = cJSON_GetObjectItem(payload, "min");
    ESP_LOGI(TAG, "min: %d", iter->valueint);
    obj[index].min = iter->valueint;

    iter = cJSON_GetObjectItem(payload, "max");
    ESP_LOGI(TAG, "max: %d", iter->valueint);
    obj[index].max = iter->valueint;
    iter = cJSON_GetObjectItem(payload,"coeff");
    cJSON * subitem = cJSON_GetArrayItem(iter, 0);
    ESP_LOGI(TAG, "value coeff[0] %f", subitem->valuedouble);
    obj[index].coeff[0] = subitem->valuedouble;
    subitem = cJSON_GetArrayItem(iter, 1);
    ESP_LOGI(TAG, "value coeff[1] %f", subitem->valuedouble);
    obj[index].coeff[1] = subitem->valuedouble;
    subitem = cJSON_GetArrayItem(iter, 2);
    ESP_LOGI(TAG, "value coeff[2] %f", subitem->valuedouble);
    obj[index].coeff[2] = subitem->valuedouble;
    subitem = cJSON_GetArrayItem(iter, 3);
    ESP_LOGI(TAG, "value coeff[3] %f", subitem->valuedouble);
    obj[index].coeff[3] = subitem->valuedouble;

    iter = cJSON_GetObjectItem(payload, "unit");
    ESP_LOGI(TAG, "unit: %s", iter->valuestring);
    strcpy(buffer, iter->valuestring);
    obj[index].unit = unit_str_id(buffer);
    ESP_LOGI(TAG, "unit index %d", obj[index].unit);
    char *resp = (obj[index].unit != -1) ? "{ \"STATUS\": \"ACK\"}" : "{ \"STATUS\": \"NACK\"}";
    httpd_resp_sendstr(req, resp);
    httpd_resp_send(req, NULL, 0);

    cJSON_Delete(payload);
    set_obj_pos_ch();
    print_obj(&obj[index]);
    return ESP_OK;
}

static esp_err_t on_default_url(httpd_req_t *req)
{
    ESP_LOGI(TAG, "URL: %s", req->uri);

    esp_vfs_spiffs_conf_t esp_vfs_spiffs_conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};
    esp_vfs_spiffs_register(&esp_vfs_spiffs_conf);

    char path[600];
    if (strcmp(req->uri, "/save") == 0)
        httpd_resp_send_404(req);
    if (strcmp(req->uri, "/") == 0)
    {
        strcpy(path, "/spiffs/index.html");
    }
    else
    {
        sprintf(path, "/spiffs%s", req->uri);
    }
    char *ext = strrchr(path, '.');
    if (strcmp(ext, ".css") == 0)
    {
        httpd_resp_set_type(req, "text/css");
    }
    if (strcmp(ext, ".js") == 0)
    {
        httpd_resp_set_type(req, "text/javascript");
    }
    if (strcmp(ext, ".png") == 0)
    {
        httpd_resp_set_type(req, "image/png");
    }

    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        httpd_resp_send_404(req);
        esp_vfs_spiffs_unregister(NULL);
        return ESP_OK;
    }
    char lineRead[256];
    while (fgets(lineRead, sizeof(lineRead), file))
    {
        httpd_resp_sendstr_chunk(req, lineRead);
    }
    httpd_resp_sendstr_chunk(req, NULL);

    esp_vfs_spiffs_unregister(NULL);
    return ESP_OK;
}

void init_server()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t save_url = {
        .uri = "/save",
        .method = HTTP_POST,
        .handler = on_save_post};
    httpd_register_uri_handler(server, &save_url);
    httpd_uri_t preview_url = {
        .uri = "/preview",
        .method = HTTP_POST,
        .handler = on_preview_post};
    httpd_register_uri_handler(server, &preview_url);
    // the last difinition, this will match everything
    httpd_uri_t default_url = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = on_default_url};
    httpd_register_uri_handler(server, &default_url);
}
