
#pragma once
#include "esp_err.h"

esp_err_t app_lcd_init();
esp_err_t app_lvgl_init();
void setBacklight ( int percent);
