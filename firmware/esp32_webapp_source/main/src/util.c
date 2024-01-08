#include <stdio.h>
#include <string.h>
#include "util.h"

#include "esp_log.h"

#define TAG "UTIL"



char *unit_id_str(int id)
{
    switch (id)
    {
    case PSI:
        return "Psi";
    case BAR:
        return "BAR";
    case CELSIUS:
        return "Celsius";
    case VOLTAGE:
        return "Voltage";
    case AFR:
        return "AFR";
    case LAMBDA:
        return "Lambda";
    case FAHRENHEIT:
        return "Fahrenheit";
    default:
        return "";
    }
}

int unit_str_id(char *u)
{
    int id = -1;
    
    if (u[0] == 'L' && u[1] == 'a' && u[2] == 'm')
    {
        id = LAMBDA;
    }
    else if (strcmp(u, "AFR") == 0)
    {
        id = AFR;
    }
    else if (u[0] == 'V' && u[1] == 'o' && u[2] == 'l')
    {
        id = VOLTAGE;
    }
    else if (strcmp(u, "BAR") == 0)
    {
        id = BAR;
    }
    else if (u[0] == 'C' && u[1] == 'e' && u[2] == 'l')
    {
        id = CELSIUS;
    }
    else if (u[0] == 'F' && u[1] == 'a' && u[2] == 'h')
    {
        id = FAHRENHEIT;
    }
    else if (strcmp(u, "Psi") == 0)
    {
        id = PSI;
    }
    return id;
}

void print_obj(struct channel_info_t *t)
{
    ESP_LOGI(TAG, "name: %s",t->name);
    ESP_LOGI(TAG, "pos: %s", t->pos);
    ESP_LOGI(TAG, "unit: %s", unit_id_str(t->unit));
}