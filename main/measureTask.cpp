/*
 * measureTask.cpp
 *
 * Created on: Aug 9, 2021
 * Author: dig
 */

#include <cstdio>
#include <ctime>
#include <string.h>
#include <math.h>
#include <sys/_timeval.h>
#include "soc/interrupts.h"

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "averager.h"
#include "guiTask.h"
#include "log.h"
#include "main.h"
#include "measureTask.h"
#include "ntc.h"
#include "settings.h"
#include "stdev.h"

#define ADDCGI

#ifdef ADDCGI
#include "mdns.h"
#endif

static const char *TAG = "measureTask";

#define TIMER_BASE_CLK                                                                                                                               \
	(APB_CLK_FREQ)									 /*!< Frequency of the clock on the input of the timer groups                                    \
													  */
#define TIMER_DIVIDER (32)							 //  Hardware timer clock divider
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER) // convert counter value to seconds

#define OFFSET 24 // counter value  measured without capacitor (at timer divider 8)

#define MAXDELTA 1.0 // if temperature delta > this value measure again.

extern float tmpTemperature;
extern int scriptState;

// static xQueueHandle gpio_evt_queue = NULL;
static QueueHandle_t gpio_evt_queue = NULL;
SemaphoreHandle_t measureSemaphore;
uint32_t gpio_num;
bool sensorDataIsSend;

uint64_t timer_counter_value;
int irqcntr;
measValues_t measValues;

Averager firstOrderAverager[NR_NTCS]; // first order
Averager logAverager[NR_NTCS];		  // second order
Averager displayAverager[NR_NTCS];
Averager refAverager(REFAVERAGES); // for reference resistor

float lastTemperature[NR_NTCS];
uint64_t refTimerValue;
const gpio_num_t NTCpins[] = {NTC1_PIN, NTC2_PIN, NTC3_PIN, NTC4_PIN};
uint8_t err;
gptimer_handle_t gptimer = NULL;

#define NRSTDEVVALUES 20

float stdevBuffer[NRSTDEVVALUES];
int nrStdValues;
uint32_t irqflag;
uint32_t IntCounter[2]; // TEST ONLY

float tmpTemperature;
// //  called when capacitor is discharged
// replaced by high prio irq assembly code

// static void IRAM_ATTR gpio_isr_handler(void *arg)
// {
// 	//	gptimer_stop( gptimer);
// 	gptimer_get_raw_count(gptimer, &timer_counter_value);
// 	xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
// 	irqcntr++;
// 	//	gpio_set_intr_type(CAP_PIN, GPIO_INTR_DISABLE);  // disable to prevent a
// 	// lot of irqs
// 	gpio_set_intr_type(COMPARATOR_PIN,
// 					   GPIO_INTR_DISABLE); // disable to prevent a lot of irqs
// }

/*
 * A simple helper function to print the raw timer counter value
 * and the counter value converted to seconds
 */
static void inline print_timer_counter(uint64_t counter_value)
{
	printf("Counter: %ld\t", (long int)counter_value);
	//	printf("Time   : %.8f s\r\n", (double) counter_value / TIMER_SCALE);
}

static void timerInit()
{
	gptimer_config_t timer_config = {
		.clk_src = GPTIMER_CLK_SRC_DEFAULT,
		.direction = GPTIMER_COUNT_UP,
		.resolution_hz = 10 * 1000 * 1000,
		.intr_priority = 3,
		.flags = 0,
	};
	ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
	ESP_ERROR_CHECK(gptimer_enable(gptimer));
	ESP_ERROR_CHECK(gptimer_start(gptimer));
}

void RegisterInt(void *you_need_this)
{
	intr_handle_t handle;
	esp_err_t err;

	gpio_config_t interrupt_pin = {
		// .intr_type=GPIO_INTR_LOW_LEVEL,
		.intr_type = GPIO_INTR_NEGEDGE,	  .pin_bit_mask = (1ULL << COMPARATOR_PIN), .mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE, .pull_down_en = GPIO_PULLDOWN_DISABLE,
	};
	ESP_ERROR_CHECK(gpio_config(&interrupt_pin));
	ESP_INTR_DISABLE(31);
	// intr_matrix_set(1, ETS_GPIO_INTR_SOURCE, 31);

	err = esp_intr_alloc(ETS_GPIO_INTR_SOURCE, ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL5, NULL, NULL, &handle);
	if (err != ESP_OK)
	{
		printf("Failure: could not install ISR, %d(0x%x)\n", err, err);
		while (1)
			vTaskDelay(500 / portTICK_PERIOD_MS);
		return;
	}

	ESP_INTR_ENABLE(31);
	while (1)
		vTaskDelay(500 / portTICK_PERIOD_MS);
}

// waits for comaparator irq
// returns true if irq was received, false if timeout
bool waitForIRQ()
{
	int timeOut = 0;
	irqflag = 0;
	while (irqflag == 0)
	{
		vTaskDelay(1);
		timeOut++;
		if (timeOut > RCTIMEOUT)
		{
			printf(" No Ref ");
			break;
		}
	}
	return irqflag > 0;
}

void IRAM_ATTR measureTask(void *pvParameters)
{
	TickType_t xLastWakeTime;
	int ntc = 0;
	int counts = 0;
	int lastminute = -1;
	int logPrescaler = LOGINTERVAL;
	time_t now;
	struct tm timeinfo;
	char line[MAXCHARSPERLINE];
	displayMssg_t displayMssg;
	displayMssg.str1 = line;
	log_t log;
	float stdev;
	gpio_num_t NTCpin;
	int oldDisplayAverages;
	int oldLogInterval;
	int timeOut = 0;

	xTaskCreatePinnedToCore(RegisterInt, "allocEXTGPIOINT", 4096, NULL, 0, NULL, 1);
	// while(1)
	vTaskDelay(10);

#ifdef SIMULATE
	float x = 0;

	ESP_LOGI(TAG, "Simulating sensors");
	while (1)
	{

		vTaskDelay(1000 / portTICK_PERIOD_MS);
		//	ESP_LOGI(TAG, "t: %1.2f RH:%1.1f co2:%f", lastVal.temperature,
		// lastVal.hum, lastVal.co2);

		//		sprintf(str, "3:%d\n\r", (int)lastVal.co2);
		//		UDPsendMssg(UDPTXPORT, str, strlen(str));
		//
		//		sprintf(str, "S: %s t:%1.2f RH:%1.1f co2:%d",
		// userSettings.moduleName, lastVal.temperature, lastVal.hum, (int)
		// lastVal.co2); 		UDPsendMssg(UDPTX2PORT, str, strlen(str));
		//
		//		ESP_LOGI(TAG, "%s %d", str, logTxIdx);
		//
		//		if (connected) {
		//			vTaskDelay(500 / portTICK_PERIOD_MS); // wait for UDP to
		// send 			sensorDataIsSend = true;
		//		}
	}

#else
	measureSemaphore = xSemaphoreCreateMutex();
	xLastWakeTime = xTaskGetTickCount();

	timerInit();
	//	gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));

	esp_rom_gpio_pad_select_gpio(CAP_PIN);
	esp_rom_gpio_pad_select_gpio(RREF_PIN);
	gpio_set_direction(COMPARATOR_PIN, GPIO_MODE_INPUT);
	gpio_set_direction(RREF_PIN, GPIO_MODE_INPUT);
	gpio_set_level(RREF_PIN, 0);
	esp_rom_gpio_pad_select_gpio(RREF_PIN);
	gpio_set_drive_capability(RREF_PIN, GPIO_DRIVE_CAP_3);

	// gpio_install_isr_service(1 << 3);
	// gpio_isr_handler_add(COMPARATOR_PIN, gpio_isr_handler, (void *)CAP_PIN);

	gpio_set_level(CAP_PIN, 1);

	for (int n = 0; n < NR_NTCS; n++)
	{
		esp_rom_gpio_pad_select_gpio(NTCpins[n]);
		gpio_set_direction(NTCpins[n], GPIO_MODE_INPUT);
		gpio_set_level(NTCpins[ntc], 0);
		logAverager[n].setAverages(AVGERAGESAMPLES);
		firstOrderAverager[n].setAverages(FIRSTORDERAVERAGES);
		//	displayAverager[n].setAverages(DISPLAYAVERAGES);
		displayAverager[n].setAverages(userSettings.middleInterval);
		lastTemperature[n] = ERRORTEMP;
		gpio_set_drive_capability(NTCpins[n], GPIO_DRIVE_CAP_3);
	}
	oldDisplayAverages = userSettings.middleInterval;
	oldLogInterval = userSettings.logInterval;

	//	while(1) {
	//		testLog();
	//		vTaskDelay(10);
	//	}

	while (1)
	{
		counts++;

		// measure reference resistor
		gpio_set_direction(CAP_PIN, GPIO_MODE_OUTPUT); // charge capacitor
		vTaskDelay(CHARGETIME);

		//		xQueueReceive(gpio_evt_queue, &gpio_num, 0);  // empty queue
		gpio_set_direction(CAP_PIN, GPIO_MODE_INPUT); // charge capacitor off
		//	gpio_set_intr_type(CAP_PIN, GPIO_INTR_NEGEDGE); // irq on again
		gpio_set_intr_type(COMPARATOR_PIN, GPIO_INTR_NEGEDGE); // irq on again

		gptimer_set_raw_count(gptimer, 0);
		gpio_set_direction(RREF_PIN, GPIO_MODE_OUTPUT); // set discharge resistor on

		waitForIRQ();

		gpio_set_direction(RREF_PIN, GPIO_MODE_INPUT); // ref off
		//	refAverager.write(timer_counter_value - OFFSET);
		//	refTimerValue = refAverager.average();
		refTimerValue = timer_counter_value - OFFSET;

		// measure NTC
		gpio_set_direction(CAP_PIN, GPIO_MODE_OUTPUT); // charge capacitor
		vTaskDelay(CHARGETIME);

		NTCpin = NTCpins[ntc];

		//	xQueueReceive(gpio_evt_queue, &gpio_num, 0);  // empty queue
		gpio_set_direction(CAP_PIN, GPIO_MODE_INPUT); // charge capacitor off
		//	gpio_set_intr_type(CAP_PIN, GPIO_INTR_NEGEDGE); // irq on again
		//		gpio_set_intr_type(COMPARATOR_PIN, GPIO_INTR_NEGEDGE); // irq on again

		gptimer_set_raw_count(gptimer, 0);
		gpio_set_direction(NTCpin, GPIO_MODE_OUTPUT); // set discharge NTC on

		//	if (xQueueReceive(gpio_evt_queue, &gpio_num, RCTIMEOUT))
		if (waitForIRQ())
		{
			gpio_set_direction(NTCpin, GPIO_MODE_INPUT); // set discharge NTC off
			//	int r = (int) ( RREF * ntcAverager[ntc].average()) / (float)
			// refTimerValue; // averaged
			uint64_t r = (uint64_t)(RREF * (timer_counter_value - OFFSET)) / (float)refTimerValue; // real time
			//	printf("%d: %d *%4d\t",ntc, refTimerValue, r);
			printf("%d: %4d\t", ntc, (int)r);
			//  printf("\t%d: r:%d %lld *%4lld ", ntc, r, refTimerValue,
			//         timer_counter_value - OFFSET);

			r -= RSERIES; //
			float temp = calcTemp(r);
			bool skip = false;
			if (temp > lastTemperature[ntc])
			{
				if ((temp - lastTemperature[ntc] > MAXDELTA))
					skip = true;
			}
			else
			{
				if (lastTemperature[ntc] - temp > MAXDELTA)
					skip = true;
			}
			if (skip)
				//	printf("*%2.3f\t", temp);
				printf("------\t");
			else
			{
				//	printf("%2.3f\t", temp);
				firstOrderAverager[ntc].write((int32_t)(temp * 1000.0));
				//	displayAverager[ntc].write((int32_t)(temp * 1000.0));
				displayAverager[ntc].write(firstOrderAverager[ntc].average());
				printf("t:%2.3f ", temp);
				//	printf("fo:%2.3f ", firstOrderAverager[ntc].average() / 1000.0);
				printf("%2.3f\t", displayAverager[ntc].average() / 1000.0);

				if (counts > 3)
				{ // skip first measurements for log
					//	logAverager[ntc].write(firstOrderAverager[ntc].average());
					logAverager[ntc].write(displayAverager[ntc].average());
					//	logAverager[ntc].write((int32_t)(temp * 1000.0));
					if (ntc == 0)
					{
						stdevBuffer[nrStdValues++] = (float)temp;
						if (nrStdValues >= NRSTDEVVALUES)
							nrStdValues = 0;
						stdev = calculateStandardDeviation(NRSTDEVVALUES, stdevBuffer);
						printf("stdev:%2.3f\t", stdev);
					}
				}
			}
			lastTemperature[ntc] = temp;
		}
		else
		{
			printf(" No NTC ");
			gpio_set_direction(NTCpins[ntc],
							   GPIO_MODE_INPUT); // set discharge NTC off

			lastTemperature[ntc] = ERRORTEMP;
		}
		gpio_set_direction(CAP_PIN, GPIO_MODE_OUTPUT); // charge capacitor
		ntc++;
		if (ntc == NR_NTCS)
		{
			ntc = 0;
			printf("\n");
			time(&now);
			localtime_r(&now, &timeinfo);
			if (lastminute != timeinfo.tm_min)
			{
				lastminute = timeinfo.tm_min; // every minute
				if (--logPrescaler <= 0)
				{
					for (int n = 0; n < NR_NTCS; n++)
					{
						log.temperature[n] = logAverager[n].average() / 1000.0;
					}
					addToLog(log);

					if (oldLogInterval != userSettings.logInterval)
					{
						oldLogInterval = userSettings.logInterval;
						saveSettings();
					}
					logPrescaler = userSettings.logInterval;
				}
			}
			displayMssg.displayItem = DISPLAY_ITEM_MEASLINE;
			displayMssg.showTime = 0;
			char fmt[6];

			snprintf(fmt, 6, "%%2.%df", userSettings.resolution);
			for (int n = 0; n < NR_NTCS; n++)
			{
				displayMssg.line = n + 1;
				if (lastTemperature[n] == ERRORTEMP)
					snprintf(line, MAXCHARSPERLINE, "-");
				else
					snprintf(line, MAXCHARSPERLINE, fmt, (displayAverager[n].average() / 1000.0) - userSettings.temperatureOffset[n]);
				xQueueReceive(displayReadyMssgBox, &displayMssg, 100);
				xQueueSend(displayMssgBox, &displayMssg, 500 / portTICK_PERIOD_MS);
				xQueueReceive(displayReadyMssgBox, &displayMssg, 100);
			}
			// vTaskDelayUntil(&xLastWakeTime, MEASINTERVAL * 1000 /
			// portTICK_PERIOD_MS);
			if (oldDisplayAverages != userSettings.middleInterval)
			{
				oldDisplayAverages = userSettings.middleInterval;
				for (int n = 0; n < NR_NTCS; n++)
					displayAverager[n].setAverages(userSettings.middleInterval);
				saveSettings();
			}
		}
		//	vTaskDelayUntil(&xLastWakeTime, MEASINTERVAL * 1000 /
		// portTICK_PERIOD_MS);
	}
#endif
}

// called from CGI
#ifdef ADDCGI
#include "cgiScripts.h"

int buildSettingsTable(char *pBuffer, int count);
int actionRespScript(char *pBuffer, int count);
int freadCGI(char *buffer, int count);
int readVarScript(char *pBuffer, int count);

int getAllLogsScript(char *pBuffer, int count);
int getNewLogsScript(char *pBuffer, int count);
int clearLogScript(char *pBuffer, int count);

bool readActionScript(char *pcParam, const CGIdesc_t *CGIdescTable, int size);
char *readCGIvalues(int iIndex, char *pcParam);

const CGIurlDesc_t CGIurls[] = {
	{"/cgi-bin/readvar", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)readVarScript},	// !!!!!! leave this index  !!
	{"/cgi-bin/writevar", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)readVarScript},	// !!!!!! leave this index  !!
	{"/action_page.php", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)actionRespScript}, // !!!!!! leave this index  !!
	{"/cgi-bin/getAllLogs", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)getAllLogsScript},
	{"/cgi-bin/getNewLogs", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)getNewLogsScript},
	{"/cgi-bin/clearLog", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)clearLogScript},
	{"/cgi-bin/getInfoValues", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)getInfoValuesScript},
	{"/cgi-bin/getCalValues", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)buildCalTable},
	{"/cgi-bin/getSettingsTable", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)buildSettingsTable},
	{"/cgi-bin/getRTMeasValues", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)getRTMeasValuesScript},
	{"/cgi-bin/saveSettings", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)saveSettingsScript},
	{"/cgi-bin/cancelSettings", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)cancelSettingsScript},
	{"", NULL, NULL}};

const CGIdesc_t settingsDescr[] = {{"MiddelInterval", &userSettings.middleInterval, INT, 1, 1, 100},
								   {"LogInterval (min)", &userSettings.logInterval, INT, 1, 1, 60},
								   {"Resolutie", &userSettings.resolution, INT, 1, 1, 3},
								   {"Modulenaam", &userSettings.moduleName, STR, 1, 1, MAX_STRLEN},
								   {"TemperatureOffset1", &userSettings.temperatureOffset[0], FLT, 1, -5, 5}, // todo
								   {"TemperatureOffset2", &userSettings.temperatureOffset[1], FLT, 1, -5, 5},
								   {"TemperatureOffset3", &userSettings.temperatureOffset[2], FLT, 1, -5, 5},
								   {"TemperatureOffset4", &userSettings.temperatureOffset[3], FLT, 1, -5, 5},
								   {NULL, NULL, INT, 0, 0, 0}};

CGIurlDesc_t *getCGIurlsTable()
{
	return (CGIurlDesc_t *)CGIurls;
}

CGIdesc_t *getSettingsDescriptorTable()
{
	return (CGIdesc_t *)settingsDescr;
}

int printLog(log_t *logToPrint, char *pBuffer)
{
	int len;
	len = sprintf(pBuffer, "%d,", (int)logToPrint->timeStamp);
	for (int n = 0; n < NR_NTCS; n++)
	{
		len += sprintf(pBuffer + len, "%3.2f,", logToPrint->temperature[n] - userSettings.temperatureOffset[n]);
	}
	len += sprintf(pBuffer + len, "\n");
	return len;
}

// // todo remove this , include with other settings
// int getSensorNameScript(char *pBuffer, int count)
// {
// 	int len = 0;
// 	switch (scriptState)
// 	{
// 	case 0:
// 		scriptState++;lastTemperature
// 		len += sprintf(pBuffer + len, "Actueel,Nieuw\n");
// 		len += sprintf(pBuffer + len, "%s\n", userSettings.moduleName);
// 		return len;
// 		break;
// 	default:
// 		break;
// 	}
// 	return 0;
// }

int getInfoValuesScript(char *pBuffer, int count)
{
	int len = 0;
	char str[10];
	switch (scriptState)
	{
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "%s\n", "Meting,Actueel,Offset");
		for (int n = 0; n < NR_NTCS; n++)
		{
			sprintf(str, "Sensor %d", n + 1);
			len += sprintf(pBuffer + len, "%s,%3.2f,%3.2f\n", str, lastTemperature[n] - userSettings.temperatureOffset[n],
						   userSettings.temperatureOffset[n]); // send values and offset
		};
		return len;
		break;
	default:
		break;
	}
	return 0;
}

// build javascript tables

int buildCalTable(char *pBuffer, int count)
{
	int len = 0;
	switch (scriptState)
	{
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "%s\n", "Meting,Referentie,Stel in,Herstel"); // header  table
		len += sprintf(pBuffer + len, "%s\n", "Sensor 1,-\n Sensor 2,-\n Sensor 3,-\n Sensor 4,-\n");
		return len;
		break;
	default:
		break;
	}
	return 0;
}
int buildSettingsTable(char *pBuffer, int count)
{
	int len = 0;
	switch (scriptState)
	{
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "%s\n", "Item,Waarde,Stel in,Herstel"); // header  table
		for (int n = 0; (settingsDescr[n].nrValues > 0); n++)
		{ // loop over all settings names
			len += sprintf(pBuffer + len, "%s,", settingsDescr[n].name);
			switch (settingsDescr[n].varType)
			{
			case INT:
				len += sprintf(pBuffer + len, "%d\n", *(int *)(settingsDescr[n].pValue));
				break;
			case FLT:
				len += sprintf(pBuffer + len, "%3.2f\n", *(float *)(settingsDescr[n].pValue));
				break;
			case STR:
				len += sprintf(pBuffer + len, "%s\n", (char *)(settingsDescr[n].pValue));
				break;
			default:
				break;
			}
		}
		return len;
		break;
	default:
		break;
	}
	return 0;
}

int saveSettingsScript(char *pBuffer, int count)
{
	saveSettings();
	return 0;
}

int cancelSettingsScript(char *pBuffer, int count)
{
	loadSettings();
	return 0;
}

calValues_t calValues = {NOCAL, NOCAL, NOCAL, NOCAL};
// @formatter:off
char tempName[MAX_STRLEN];

const CGIdesc_t writeVarDescriptors[] = {{"Temperatuur", &calValues.temperature, FLT, NR_NTCS}, {"moduleName", tempName, STR, 1}};

#define NR_CALDESCRIPTORS (sizeof(writeVarDescriptors) / sizeof(CGIdesc_t))
// @formatter:on

int getRTMeasValuesScript(char *pBuffer, int count)
{
	int len = 0;

	switch (scriptState)
	{
	case 0:
		scriptState++;

		char fmt[7];
		snprintf(fmt, 7, "%%2.%df,", userSettings.resolution);

		len = sprintf(pBuffer + len, "%u,", (unsigned int)timeStamp++);
		for (int n = 0; n < NR_NTCS; n++)
		{
			len += sprintf(pBuffer + len, fmt, (displayAverager[n].average() / 1000.0) - userSettings.temperatureOffset[n]);
		}
		len += sprintf(pBuffer + len, "\n");
		return len;
		break;
	default:
		break;
	}
	return 0;
}

void parseCGIWriteData(char *buf, int received)
{
	parseCGIsettings(buf, received);
}

#endif
