/*
 * guiTask.c
 *
 *  Created on: Mar 2, 2021
 *      Author: dig
 *
 *      handles screens
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "guiTask.h"
//#include "TFT_eSPI.h"

QueueHandle_t displayMssgBox;
QueueHandle_t displayReadyMssgBox;

volatile bool displayReady;

#define LINESPACING 10


void guiTask(void *pvParameter) {
	displayMssg_t recDdisplayMssg;
	int dummy;
	int step = 0;
	char *str;
	int fontHeight;
	int ypos;

	displayMssgBox = xQueueCreate(2, sizeof(displayMssg_t));
	displayReadyMssgBox = xQueueCreate(1, sizeof(uint32_t));

	while (1)
		vTaskDelay(100);
//
//	TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
//
//	tft.init();
//	tft.setRotation(3);
//	tft.fillRect(0, 0, 240, 240, TFT_BLACK); // clear screen

//	displayReady = true;
//	while (1) {
//		if (xQueueReceive(displayMssgBox,(void * const) &recDdisplayMssg, portMAX_DELAY) == pdTRUE) {
//			if( recDdisplayMssg.line < 5)
//			{
//				fontHeight = 20;
//				tft.setFreeFont(&FreeMono18pt7b);
//			}
//			else {
//				fontHeight = 15;
//				tft.setFreeFont(&FreeMono12pt7b);
//			}
//			ypos = (recDdisplayMssg.line) * (fontHeight + LINESPACING);
//
//			tft.fillRect(0, ypos, 240, fontHeight+ LINESPACING,   TFT_BLACK); // clear line
//			tft.setCursor(0,ypos + fontHeight);
//		    tft.print(recDdisplayMssg.str1);
//			if ( recDdisplayMssg.showTime)
//				vTaskDelay(recDdisplayMssg.showTime/portTICK_PERIOD_MS);
//			xQueueSend(displayReadyMssgBox, &dummy, 0);
//		}
//	}
}


// /home/dig/.espressif/tools/openocd-esp32/v0.10.0-esp32-20200420/openocd-esp32/bin/openocd -f interface/ftdi/c232hm.cfg -f board/esp-wroom-32.cfg -c "program_esp /home/dig/projecten/littleVGL/dmmGui/build/dmm. 0x10000 verify"
