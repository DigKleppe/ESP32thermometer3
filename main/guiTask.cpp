/*
 * guiTask.c
 *
 *  Created on: Mar 2, 2021
 *      Author: dig
 *
 *      handles screens
 */

#include "guiTask.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "LCD.h"
#include "lvgl.h"
#include <stdio.h>

static const char *TAG = "guiTask";


QueueHandle_t displayMssgBox;
QueueHandle_t displayReadyMssgBox;
volatile bool displayReady;



#define COLL1WIDTH 50
#define COLL2WIDTH 160
#define COLL3WIDTH 65
#define ROW1HEIGHT 40

static lv_obj_t *measvalueLabel[4];
static lv_obj_t *infoLabel;

#define FONT &lv_font_montserrat_46
#define INFOFONT &lv_font_montserrat_16
#define BG_COLOR  lv_color_hex(0xEEFF41)
#define FONT_COLOR  lv_color_hex(0x1565C0) 

void buildMainScreen(void) {

	static lv_style_t style1;
	static lv_style_t style2;

	static int32_t col_dsc[] = {COLL1WIDTH, COLL2WIDTH, COLL3WIDTH, LV_GRID_TEMPLATE_LAST};
	static int32_t row_dsc[] = {ROW1HEIGHT, ROW1HEIGHT, ROW1HEIGHT, ROW1HEIGHT, LV_GRID_TEMPLATE_LAST};

	lvgl_port_lock(0);

	lv_obj_set_style_bg_color(lv_screen_active(), BG_COLOR, LV_PART_MAIN);

	int screenWidth = lv_obj_get_width(lv_screen_active());

	lv_style_init(&style1); // for temperature values
	//	lv_style_set_bg_color(&style1, lv_co // lv_color_hex(0xa03080));
	//	lv_style_set_border_width(&style1, 2);
	//	lv_style_set_border_color(&style1, lv_color_hex(0x000000));
	lv_style_set_text_color(&style1, FONT_COLOR);
	lv_style_set_text_font(&style1, FONT);

	lv_style_init(&style2);
	lv_style_set_bg_color(&style2, BG_COLOR);
	lv_style_set_border_width(&style2, 2);
	lv_style_set_border_color(&style2, FONT_COLOR);
	lv_style_set_text_color(&style2, FONT_COLOR);
	lv_style_set_text_font(&style2, INFOFONT);

	/*Create a container with grid*/
	lv_obj_t *cont = lv_obj_create(lv_screen_active());
	lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
	lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);
	lv_obj_set_size(cont, screenWidth, 220);
	lv_obj_set_style_bg_color(cont, BG_COLOR, LV_PART_MAIN);
	lv_obj_set_align(cont, LV_ALIGN_TOP_MID);
	//	lv_obj_center(cont);
	lv_obj_set_layout(cont, LV_LAYOUT_GRID);

	lv_obj_t *label;
	int row = 0;

	do {
		label = lv_label_create(cont); // create channelnumber label
		lv_obj_add_style(label, &style1, 0);
		lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, row, 1);
		lv_label_set_text_fmt(label, "%d:", row + 1);
		lv_obj_center(label);
		measvalueLabel[row] = lv_label_create(cont); // create value label
		lv_obj_add_style(measvalueLabel[row], &style1, 0);
		lv_obj_set_grid_cell(measvalueLabel[row], LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, row, 1);
		lv_label_set_text_fmt(measvalueLabel[row], "----");
		lv_obj_center(measvalueLabel[row]);
		label = lv_label_create(cont);
		lv_obj_add_style(label, &style1, 0); // create degrees  C
		lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, row, 1);
		lv_label_set_text_fmt(label, "°C");
		lv_obj_center(label);
		row++;
	} while (row_dsc[row] != LV_GRID_TEMPLATE_LAST);

	infoLabel = lv_label_create(lv_screen_active());
	lv_obj_add_style(infoLabel, &style2, 0);
	lv_obj_set_align(infoLabel, LV_ALIGN_BOTTOM_MID);
	lv_label_set_text(infoLabel, "");
	lv_obj_set_width(infoLabel, screenWidth);

	lvgl_port_unlock();
}

void guiTask(void *pvParameter) {
	displayMssg_t recDdisplayMssg;
	int dummy = 0;

	displayMssgBox = xQueueCreate(1, sizeof(displayMssg_t));
	displayReadyMssgBox = xQueueCreate(1, sizeof(uint32_t));

 /* LCD HW initialization */
  ESP_ERROR_CHECK(app_lcd_init());

  /* LVGL initialization */
  ESP_ERROR_CHECK(app_lvgl_init());


	buildMainScreen();

	displayReady = true;
	while (1) {
		if (xQueueReceive(displayMssgBox, (void *const)&recDdisplayMssg, portMAX_DELAY) == pdTRUE) {
			int line = recDdisplayMssg.line-1;
			lvgl_port_lock(0);
			if (line <= 3) {
				lv_label_set_text_fmt(measvalueLabel[line], recDdisplayMssg.str1);
			} else {
				lv_label_set_text(infoLabel, recDdisplayMssg.str1);
			}
			lvgl_port_unlock();
			if (recDdisplayMssg.showTime)
				vTaskDelay(recDdisplayMssg.showTime / portTICK_PERIOD_MS);
			xQueueSend(displayReadyMssgBox, &dummy, 0);
		}
	}
}
