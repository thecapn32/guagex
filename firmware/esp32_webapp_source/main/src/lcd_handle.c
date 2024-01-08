#include <stdio.h>
#include "sdkconfig.h"
#include <math.h>

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
#include "mdns.h"

#include "driver/adc.h"

#include "cJSON.h"

#include "protocol_examples_common.h"

#include <u8g2.h>
#include "u8g2_esp32_hal.h"

#include "util.h"
#include "webserver.h"
#include "lcd_handle.h"

/**
 *
 * ADC PINOUT CONFIG
 *
 * CH1 --> IO13 ADC2_CHANNEL_3
 * CH2 --> IO12 ADC2_CHANNEL_2
 * CH3 --> IO11 ADC2_CHANNEL_1
 * CH4 --> IO10 ADC1_CHANNEL_9
 * CH5 -->  IO9 ADC1_CHANNEL_8
 * CH6 -->  IO8 ADC1_CHANNEL_7
 * CH7 -->  IO7 ADC1_CHANNEL_6
 * CH8 -->  IO6 ADC1_CHANNEL_5
 */

/*
CH0 -- GPIO13 -- ADC2_CH2
CH1 -- GPIO12 -- ADC2_CH1
CH2 -- GPIO11 -- ADC2_CH0
CH3 -- GPIO10 -- ADC1_CH9
CH4 -- GPIO9 -- ADC1_CH8
CH5 -- GPIO8 -- ADC1_CH7
CH6 -- GPIO7 -- ADC1_CH6
CH7 -- GPIO6 -- ADC1_CH5
*/

#define TAG "LCD_SSD1322"

const adc2_channel_t ch1 = ADC2_CHANNEL_2;
const adc2_channel_t ch2 = ADC2_CHANNEL_1;
const adc2_channel_t ch3 = ADC2_CHANNEL_0;

const adc1_channel_t ch4 = ADC1_CHANNEL_9;
const adc1_channel_t ch5 = ADC1_CHANNEL_8;
const adc1_channel_t ch6 = ADC1_CHANNEL_7;
const adc1_channel_t ch7 = ADC1_CHANNEL_6;
const adc1_channel_t ch8 = ADC1_CHANNEL_5;

const adc_bits_width_t width = ADC_WIDTH_BIT_12;
const adc_atten_t atten = ADC_ATTEN_DB_11;

static u8g2_t u8g2;

extern int pos_ch[8];
extern struct channel_info_t obj[8];

static void draw_grids()
{
    u8g2_DrawLine(&u8g2, 0, 31, 255, 31);
    u8g2_DrawLine(&u8g2, 127, 0, 127, 63);
    u8g2_DrawLine(&u8g2, 63, 0, 63, 63);
    u8g2_DrawLine(&u8g2, 191, 0, 191, 63);

    u8g2_DrawLine(&u8g2, 0, 0, 0, 63);     // | left
    u8g2_DrawLine(&u8g2, 255, 0, 255, 63); // | right
    u8g2_DrawLine(&u8g2, 0, 0, 255, 0);    // _ up
    u8g2_DrawLine(&u8g2, 0, 63, 255, 63);  // _ down
    u8g2_SendBuffer(&u8g2);
}

static void update_pos_display(double value, char *label, enum unit_t unit, char pos, int max, int min)
{
    u8g2_uint_t x, y = 0;

    switch (pos)
    {
    case 'A':
        x = 1;
        y = 1;
        break;
    case 'B':
        x = 64;
        y = 1;
        break;
    case 'C':
        x = 128;
        y = 1;
        break;
    case 'D':
        x = 192;
        y = 1;
        break;
    case 'E':
        x = 1;
        y = 32;
        break;
    case 'F':
        x = 64;
        y = 32;
        break;
    case 'G':
        x = 128;
        y = 32;
        break;
    case 'H':
        x = 192;
        y = 32;
        break;
    default:
        ESP_LOGE(TAG, "Invalid display position for lcd module %c", pos);
        return;
        break;
    }
    
    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_DrawBox(&u8g2, x, y, 62, 30); // this is for section clearnce
    u8g2_SetDrawColor(&u8g2, 1);
    char template[50];
    if(value >= min && value <= max)
        sprintf(template, "%.01f", value);
    else 
        sprintf(template, "%.01f!", value);
    //ESP_LOGI("TAG", "sensor value %s pos: %c label:%s", template, pos, label);
    u8g2_SetFont(&u8g2, u8g2_font_ncenR12_tr);
    if(strlen(template) <= 5)
        u8g2_DrawStr(&u8g2, x + 2, y + 14, template); // display the number
    else
        u8g2_DrawStr(&u8g2, x + 2, y + 14, "xxx");
    
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    char *k = unit_id_str(unit);
    char k1[100];
    strcpy(k1, k);
    if(strlen(k1) > 3)
        k1[3] = '\0';
    u8g2_DrawStr(&u8g2, x + 41, y + 14, k1); // display the unit
    u8g2_SetFont(&u8g2, u8g2_font_ncenR08_tr);
    u8g2_DrawStr(&u8g2, x + 2, y + 26, label);
    u8g2_SendBuffer(&u8g2);
}

static void setup_lcd()
{
    u8g2_esp32_hal_t u8g2_esp32_hal; //= U8G2_ESP32_HAL_DEFAULT;
    u8g2_esp32_hal.clk = GPIO_NUM_18;
    u8g2_esp32_hal.mosi = GPIO_NUM_17;
    u8g2_esp32_hal.cs = GPIO_NUM_41;
    u8g2_esp32_hal.dc = GPIO_NUM_42;
    u8g2_esp32_hal.reset = GPIO_NUM_38;
    u8g2_esp32_hal.sda = GPIO_NUM_NC;
    u8g2_esp32_hal.scl = GPIO_NUM_NC;
    u8g2_esp32_hal_init(u8g2_esp32_hal);
    
    u8g2_Setup_ssd1322_nhd_256x64_f(&u8g2,
                                    U8G2_R0,
                                    u8g2_esp32_spi_byte_cb,
                                    u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure

    u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in sleep mode after this,

    u8g2_SetPowerSave(&u8g2, 0); // wake up display
    u8g2_ClearBuffer(&u8g2);
    u8g2_SendBuffer(&u8g2);
    draw_grids(&u8g2);
    
    // update_pos_display(33, "Water lvl", 0, 'A');
    // update_pos_display(33, "Water lvl", 1, 'B');
    // update_pos_display(33, "Water lvl", 2, 'C');
    // update_pos_display(33, "Water lvl", 3, 'D');
    // update_pos_display(33, "Water lvl", 4, 'E');
    // update_pos_display(33, "Water lvl", 5, 'F');
    // update_pos_display(33, "Water lvl", 6, 'G');
    // update_pos_display(33, "Water lvl", 6, 'H');
}

static void config_adc_channels()
{
    adc1_config_width(width);
    
    adc1_config_channel_atten(ch4, atten);
    adc1_config_channel_atten(ch5, atten);
    adc1_config_channel_atten(ch7, atten);
    adc1_config_channel_atten(ch8, atten);
    adc2_config_channel_atten(ch1, atten);
    adc2_config_channel_atten(ch2, atten);
    adc2_config_channel_atten(ch3, atten);
}

static int cal_voltage(int val)
{
    return val * 3100 / 4095;
}

static void adc_channel_sample_all()
{
    int adc_r = 0;
    ESP_LOGI("ADC", "-----");
    adc2_get_raw((adc2_channel_t)ch1, width, &adc_r);
    ESP_LOGI("ADC", "CH0: %d mv", cal_voltage(adc_r));
    adc2_get_raw((adc2_channel_t)ch2, width, &adc_r);
    ESP_LOGI("ADC", "CH1: %d mv", cal_voltage(adc_r));
    adc2_get_raw((adc2_channel_t)ch3, width, &adc_r);
    ESP_LOGI("ADC", "CH2: %d mv", cal_voltage(adc_r));
    adc_r = adc1_get_raw((adc1_channel_t)ch4);
    ESP_LOGI("ADC", "CH3: %d mv", cal_voltage(adc_r));
    adc_r = adc1_get_raw((adc1_channel_t)ch5);
    ESP_LOGI("ADC", "CH4: %d mv", cal_voltage(adc_r));
    adc_r = adc1_get_raw((adc1_channel_t)ch6);
    ESP_LOGI("ADC", "CH5: %d mv", cal_voltage(adc_r));
    adc_r = adc1_get_raw((adc1_channel_t)ch7);
    ESP_LOGI("ADC", "CH6: %d mv", cal_voltage(adc_r));
    adc_r = adc1_get_raw((adc1_channel_t)ch8);
    ESP_LOGI("ADC", "CH7: %d mv", cal_voltage(adc_r));
    
}

static int adc_channel_sample(int channel)
{
    int adc_r = 0;
    switch (channel)
    {
    case 0:
        adc2_get_raw((adc2_channel_t)ch1, width, &adc_r);
        return adc_r;
    case 1:
        adc2_get_raw((adc2_channel_t)ch2, width, &adc_r);
        return adc_r;
    case 2:
        adc2_get_raw((adc2_channel_t)ch3, width, &adc_r);
        return adc_r;
    case 3:
        return adc1_get_raw((adc1_channel_t)ch4);
    case 4:
        return adc1_get_raw((adc1_channel_t)ch5);
    case 5:
        return adc1_get_raw((adc1_channel_t)ch6);
    case 6:
        return adc1_get_raw((adc1_channel_t)ch7); 
    case 7:
        return adc1_get_raw((adc1_channel_t)ch8); 
    default:
        ESP_LOGE("ADC_SAMPLE", "Invalid channel number");
        return -1;
    }
}

static double cal_sensor_value(int adc_raw, double coef[4])
{
    double val = 0;
    val = (coef[0] * pow(adc_raw, 3)) + (coef[1] * pow(adc_raw, 2)) + (coef[2]* adc_raw) + (coef[3]);
    return val;
}

void lcd_handle_task(void *params)
{
    setup_lcd();
    config_adc_channels();
    adc_channel_sample_all();
    int adc_raw = 0;
    while (1)
    {
        // wait for 1 second then sample
        vTaskDelay(pdMS_TO_TICKS(3000));
        //adc_channel_sample_all();
        for(int i = 0; i < 8; i++)
        {
            if(pos_ch[i] != -1)
            {
                int channel_index = pos_ch[i];
                adc_raw = adc_channel_sample(channel_index);
                //ESP_LOGI("ADC", "%dth sample value %d", i, adc_raw); 
                update_pos_display(cal_sensor_value(adc_raw ,obj[channel_index].coeff),
                obj[channel_index].name , obj[channel_index].unit, 'A' + i,obj[channel_index].max ,obj[channel_index].min);
            }
        }
    }
}