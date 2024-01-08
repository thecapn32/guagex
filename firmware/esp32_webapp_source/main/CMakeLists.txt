idf_component_register(SRCS "src/main.c" "src/u8g2_esp32_hal.c" "src/webserver.c" "src/util.c" "src/lcd_handle.c" "src/flash_nvm.c"
                    INCLUDE_DIRS "include")
                    
spiffs_create_partition_image(spiffs ../esp_site FLASH_IN_PROJECT)