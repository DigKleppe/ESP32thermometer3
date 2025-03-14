/*
 * measureTask.h
 *
 *  Created on: Aug 9, 2021
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_MEASURETASK_H_
#define MAIN_INCLUDE_MEASURETASK_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define NR_NTCS 4
#include "settings.h"

// #define SIMULATE 		1

#define REFAVERAGES 3
#define DISPLAYAVERAGES 8

#define RREF 10000.0 // in ohms

// #define RREF 10071.0 // in ohms

#define CAP_PIN GPIO_NUM_4
#define COMPARATOR_PIN GPIO_NUM_1 // change also in irq.S !

#define RREF_PIN GPIO_NUM_5

// #define NTC1_PIN GPIO_NUM_6
// #define NTC2_PIN GPIO_NUM_7
// #define NTC3_PIN GPIO_NUM_17
// #define NTC4_PIN GPIO_NUM_18

// test for system , measuring same resistor
//  #define NTC1_PIN		GPIO_NUM_6
//  #define NTC2_PIN		GPIO_NUM_6//7
//  #define NTC3_PIN		GPIO_NUM_6//   17
//  #define NTC4_PIN		GPIO_NUM_6//18

#define NTC1_PIN GPIO_NUM_5
#define NTC2_PIN GPIO_NUM_5 // 7
#define NTC3_PIN GPIO_NUM_5 //   17
#define NTC4_PIN GPIO_NUM_5 // 18

#define RSERIES 270 // resistors in series with NTC

#define CHARGETIME 200 // ms
#define RCTIMEOUT 300  // ms

#define MEASINTERVAL 1 // interval for sensor in seconds
#define LOGINTERVAL 5  // minutes
#define AVGERAGESAMPLES ((LOGINTERVAL * 60) / (MEASINTERVAL))
#define FIRSTORDERAVERAGES 4

#define NOCAL 99999

void measureTask(void *pvParameters);

// extern float temperature[NR_NTCS];
extern float refTemperature;
extern bool sensorDataIsSend;

extern SemaphoreHandle_t measureSemaphore; // to prevent from small influences on IRQ

typedef struct
{
	float temperature[NR_NTCS];
} calValues_t;

int getRTMeasValuesScript(char *pBuffer, int count);
int getNewMeasValuesScript(char *pBuffer, int count);
int getLogScript(char *pBuffer, int count);
int getInfoValuesScript(char *pBuffer, int count);
int buildCalTable(char *pBuffer, int count);
int saveSettingsScript(char *pBuffer, int count);
int cancelSettingsScript(char *pBuffer, int count);
int calibrateRespScript(char *pBuffer, int count);
int getSensorNameScript(char *pBuffer, int count);
void parseCGIWriteData(char *buf, int received);

CGIdesc_t *getSettingsDescriptorTable();

#endif /* MAIN_INCLUDE_MEASURETASK_H_ */
