#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "util.h"

#define STORAGE_NAMESPACE "storage"

extern struct channel_info_t obj[8];
extern int pos_ch[8];

void init_flash()
{
    nvs_handle_t my_handle;
    esp_err_t err;
    int init;
    err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    err = nvs_get_i32(my_handle, "initialize", &init);

    if (err != ESP_OK)
    {
        ESP_LOGI("nvs_flash", "setting values for the first time");
        init = 0;
        nvs_set_i32(my_handle, "initialize", init);
        nvs_set_blob(my_handle, "obj", &obj, sizeof(obj));
        for (int i = 0; i < 8; i++)
        {
            pos_ch[i] = -1;
        }
        nvs_set_blob(my_handle, "pos_ch", &pos_ch, sizeof(pos_ch));
    }
    else
    {
        size_t init1;
        ESP_LOGI("nvs_flash", "reading previously stored values ftrom flash memory");
        err = nvs_get_blob(my_handle, "obj", NULL, &init1);
        err = nvs_get_blob(my_handle, "obj", &obj, &init1);
        if(err != ESP_OK)
            ESP_LOGE("nvs_flash", "error reading obj from flash");
        err = nvs_get_blob(my_handle, "pos_ch", NULL, &init1);
        err = nvs_get_blob(my_handle, "pos_ch", &pos_ch, &init1);
        if(err != ESP_OK)
            ESP_LOGE("nvs_flash", "error reading pos_ch from flash");
    }
}

void set_obj_pos_ch()
{
    nvs_handle_t my_handle;
    esp_err_t err;
    int init;
    err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    ESP_LOGI("nvs_flash", "store update config values in flash");
    err = nvs_set_blob(my_handle, "obj", &obj, sizeof(obj));
    if(err != ESP_OK)
        ESP_LOGE("nvs_flash", "error writing pos_ch from flash");
    err = nvs_set_blob(my_handle, "pos_ch", &pos_ch, sizeof(pos_ch));
    if(err != ESP_OK)
        ESP_LOGE("nvs_flash", "error writing obj from flash");

}