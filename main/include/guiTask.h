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
extern volatile bool displayReady;

typedef struct {
  char *text;
  int line;
  int showTime;
} displayMssg_t;


extern "C" {

void guiTask(void *pvParameter);
}

#endif /* COMPONENTS_GUI_GUITASK_H_ */
