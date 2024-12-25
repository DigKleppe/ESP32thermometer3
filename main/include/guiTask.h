/*
 * guiTask.h
 *
 *  Created on: Apr 17, 2021
 *      Author: dig
 */

#ifndef COMPONENTS_GUI_GUITASK_H_
#define COMPONENTS_GUI_GUITASK_H_
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern QueueHandle_t displayMssgBox;
extern QueueHandle_t displayReadyMssgBox;

typedef enum {
	DISPLAY_ITEM_STATUSLINE, DISPLAY_ITEM_MEASLINE, DISPLAY_ITEM_MESSAGE, DISPLAY_ITEM_COLOR,  DISPLAY_ITEM_STOP
} displayItem_t;

typedef struct {
	displayItem_t displayItem;
	int line;
	int showTime;
	char *str1;
} displayMssg_t;

extern volatile bool displayReady;

#define MAXCHARSPERLINE 11
#define MAXLINES		8

typedef struct {
	char line [MAXCHARSPERLINE +1];
}line_t;

extern "C" {

void guiTask(void *pvParameter);

}




#endif /* COMPONENTS_GUI_GUITASK_H_ */
