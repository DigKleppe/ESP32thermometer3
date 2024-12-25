/*
 * measureTask.cpp
 *
 * Created on: Aug 9, 2021
 * Author: dig
 */
#include <string.h>

#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gptimer.h"
//#include "driver/timer.h"
#include "driver/gpio.h"
#include "esp_log.h"
//#include "driver/i2c.h"
#include "mdns.h"

#include "ntc.h"
#include "averager.h"
#include "main.h"
#include "measureTask.h"
//#include "i2cTask.h"
#include "guiTask.h"
#include "log.h"
#include "stdev.h"

#define ADDCGI
static const char *TAG = "measureTask";

#define TIMER_BASE_CLK			(APB_CLK_FREQ)  /*!< Frequency of the clock on the input of the timer groups */
#define TIMER_DIVIDER         	(32)  //  Hardware timer clock divider
#define TIMER_SCALE           	(TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define CHARGETIME 			  	10

#define OFFSET 50 // counter value  measured without capacitor ( at timer divder 8)

#define MAXDELTA	0.4  // if temperature delta > this value measure again.

extern float tmpTemperature;
extern int scriptState;

//static xQueueHandle gpio_evt_queue = NULL;
static QueueHandle_t gpio_evt_queue = NULL;
SemaphoreHandle_t measureSemaphore;
uint32_t gpio_num;
bool sensorDataIsSend;

uint64_t timer_counter_value;
int irqcntr;
measValues_t measValues;
//log_t tLog[ MAXLOGVALUES];
////static log_t lastVal;
//int logTxIdx;
//int logRxIdx;

Averager firstOrderAverager[NR_NTCS]; // first order
Averager logAverager[NR_NTCS]; // second order
Averager displayAverager[NR_NTCS];

Averager refAverager(REFAVERAGES);  // for reference resistor


Averager refSensorAverager(AVGERAGESAMPLES);
float lastTemperature[NR_NTCS];
float refTimerValue;
const gpio_num_t NTCpins[] =
{ NTC1_PIN, NTC2_PIN, NTC3_PIN, NTC4_PIN };
uint8_t err;
gptimer_handle_t gptimer = NULL;

#define NRSTDEVVALUES 20

float stdevBuffer[NRSTDEVVALUES];
int nrStdValues;

float tmpTemperature;
//  called when capacitor is discharged
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
	gptimer_get_raw_count(gptimer, &timer_counter_value);
	xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
	irqcntr++;
	gpio_set_intr_type(CAP_PIN, GPIO_INTR_DISABLE);  // disable to prevent a lot of irqs
}

/*
 * A simple helper function to print the raw timer counter value
 * and the counter value converted to seconds
 */
static void inline print_timer_counter(uint64_t counter_value)
{
	printf("Counter: %ld\t", (long int) counter_value);
//	printf("Time   : %.8f s\r\n", (double) counter_value / TIMER_SCALE);
}

static void timerInit()
{
	gptimer_config_t timer_config =
	{ .clk_src = GPTIMER_CLK_SRC_DEFAULT, .direction = GPTIMER_COUNT_UP, .resolution_hz = 6 * 1000 * 1000, .flags = 0, };
	ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
	ESP_ERROR_CHECK(gptimer_enable(gptimer));
	ESP_ERROR_CHECK(gptimer_start(gptimer));
}

void measureTask(void *pvParameters)
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

#ifdef SIMULATE
	float x = 0;

	ESP_LOGI(TAG, "Simulating sensors");
	while (1) {


		vTaskDelay( 1000/portTICK_PERIOD_MS);
		//	ESP_LOGI(TAG, "t: %1.2f RH:%1.1f co2:%f", lastVal.temperature, lastVal.hum, lastVal.co2);

//		sprintf(str, "3:%d\n\r", (int)lastVal.co2);
//		UDPsendMssg(UDPTXPORT, str, strlen(str));
//
//		sprintf(str, "S: %s t:%1.2f RH:%1.1f co2:%d", userSettings.moduleName, lastVal.temperature, lastVal.hum, (int) lastVal.co2);
//		UDPsendMssg(UDPTX2PORT, str, strlen(str));
//
//		ESP_LOGI(TAG, "%s %d", str, logTxIdx);
//
//		if (connected) {
//			vTaskDelay(500 / portTICK_PERIOD_MS); // wait for UDP to send
//			sensorDataIsSend = true;
//		}
	}

#else
	measureSemaphore = xSemaphoreCreateMutex();
	xLastWakeTime = xTaskGetTickCount();

	timerInit();
	gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));

	esp_rom_gpio_pad_select_gpio (CAP_PIN);
	esp_rom_gpio_pad_select_gpio (RREF_PIN);
	gpio_set_direction(RREF_PIN, GPIO_MODE_INPUT);
	gpio_set_level(RREF_PIN, 0);
	esp_rom_gpio_pad_select_gpio(RREF_PIN);
	gpio_set_drive_capability(RREF_PIN, GPIO_DRIVE_CAP_3);

	gpio_install_isr_service(1 << 3);
	gpio_isr_handler_add(CAP_PIN, gpio_isr_handler, (void*) CAP_PIN);

	for (int n = 0; n < NR_NTCS; n++)
	{
		esp_rom_gpio_pad_select_gpio(NTCpins[n]);
		gpio_set_direction(NTCpins[n], GPIO_MODE_INPUT);
		gpio_set_level(NTCpins[ntc], 0);
		logAverager[n].setAverages(AVGERAGESAMPLES);
		firstOrderAverager[n].setAverages(FIRSTORDERAVERAGES);
		displayAverager[n].setAverages(DISPLAYAVERAGES);
		lastTemperature[n] = ERRORTEMP;
		gpio_set_drive_capability(NTCpins[n], GPIO_DRIVE_CAP_3);
	}
//	while(1) {
//		testLog();
//		vTaskDelay(10);
//	}
	while (1)
	{
		counts++;
		//ntc = 0;
		// measure reference resistor
		gpio_set_direction(CAP_PIN, GPIO_MODE_OUTPUT); // charge capacitor
		gpio_set_level(CAP_PIN, 1);

		vTaskDelay(CHARGETIME);
		gpio_set_direction(RREF_PIN, GPIO_MODE_OUTPUT);  // set discharge resistor on

		gpio_set_intr_type(CAP_PIN, GPIO_INTR_NEGEDGE);
		if (xSemaphoreTake(measureSemaphore, portMAX_DELAY) == pdTRUE)
		{
			gptimer_set_raw_count(gptimer, 0);
			gpio_set_direction(CAP_PIN, GPIO_MODE_INPUT); // discharge capacitor
			xQueueReceive(gpio_evt_queue, &gpio_num, 0);
			if (xQueueReceive(gpio_evt_queue, &gpio_num, 500))
			{ // portMAX_DELAY)) {
		//		print_timer_counter(timer_counter_value);
				;
			}
			else
				printf(" No Ref ");
			xSemaphoreGive(measureSemaphore);
		}

		refAverager.write(timer_counter_value - OFFSET);
		refTimerValue = refAverager.average();
		//	refTimerValue = timer_counter_value - OFFSET;
		gpio_set_direction(RREF_PIN, GPIO_MODE_INPUT);
		// measure NTC
		gpio_set_direction(CAP_PIN, GPIO_MODE_OUTPUT); // charge capacitor
		gpio_set_level(CAP_PIN, 1);

		vTaskDelay(CHARGETIME);
		gpio_set_intr_type(CAP_PIN, GPIO_INTR_NEGEDGE); // irq on again
		gpio_set_level(NTCpins[ntc], 0);
		gpio_set_direction(NTCpins[ntc], GPIO_MODE_OUTPUT);  // set discharge NTC on

		xQueueReceive(gpio_evt_queue, &gpio_num, 0);

		if (xSemaphoreTake(measureSemaphore, portMAX_DELAY) == pdTRUE)
		{
			//	timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);
			gptimer_set_raw_count(gptimer, 0);
			gpio_set_direction(CAP_PIN, GPIO_MODE_INPUT); // charge capacitor off

			if (xQueueReceive(gpio_evt_queue, &gpio_num, 500))
			{ // portMAX_DELAY)) {

				//	int r = (int) ( RREF * ntcAverager[ntc].average()) / (float) refTimerValue; // averaged
				int r = (int) ( RREF * timer_counter_value - OFFSET) / (float) refTimerValue; // real time
				printf("*%4d\t", r);

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
					;
				else
				{
				//	printf("%2.3f\t", temp);
					firstOrderAverager[ntc].write((int32_t)(temp * 1000.0));
				//	displayAverager[ntc].write((int32_t)(temp * 1000.0));
					displayAverager[ntc].write(firstOrderAverager[ntc].average());

					printf("%d: %2.2f\t",ntc, displayAverager[ntc].average()/1000.0);

					if (counts > 5)
					{ // skip first measurements for log
						logAverager[ntc].write(firstOrderAverager[ntc].average());
					//	logAverager[ntc].write((int32_t)(temp * 1000.0));
						if (ntc == 0)
						{
							stdevBuffer[nrStdValues++] = (float) temp;
							if (nrStdValues >= NRSTDEVVALUES)
								nrStdValues = 0;
							stdev = calculateStandardDeviation(NRSTDEVVALUES,stdevBuffer);
							printf("stdev:%2.2f\t", stdev);
						}
					}
				}
				lastTemperature[ntc] = temp;
			}
			else
			{
				printf(" No NTC ");
				lastTemperature[ntc] = ERRORTEMP;
			}
			xSemaphoreGive(measureSemaphore);
		}

		gpio_set_direction(NTCpins[ntc], GPIO_MODE_INPUT);  // set discharge NTC off

		ntc++;
		if (ntc == NR_NTCS)
		{
			ntc = 0;
			printf("\n");
			refSensorAverager.write(tmpTemperature * 1000.0);
			time(&now);
			localtime_r(&now, &timeinfo);  // no use in low power mode
			if (lastminute != timeinfo.tm_min)
			{
				lastminute = timeinfo.tm_min;   // every minute
				if (logPrescaler-- == 0)
				{
					logPrescaler = LOGINTERVAL;

					for (int n = 0; n < NR_NTCS; n++)
					{
						log.temperature[n] = logAverager[n].average() / 1000.0;
					}
				//	log.refTemperature = refSensorAverager.average() / 1000.0; // from I2C TMP117
				log.refTemperature = 0.0;
					addToLog(log);
				}
			}

			displayMssg.showTime = 0;
			for (int n = 0; n < NR_NTCS; n++)
			{
				displayMssg.line = n;
				if (lastTemperature[n] == ERRORTEMP)
					snprintf(line, MAXCHARSPERLINE, "t%d : -", n + 1);
				else
			//		snprintf(line, MAXCHARSPERLINE, "t%d : %2.2f", n + 1, lastTemperature[n]);
					snprintf(line, MAXCHARSPERLINE, "t%d : %2.2f", n + 1, displayAverager[n].average()/1000.0) ;

				xQueueSend(displayMssgBox, &displayMssg, 0);
				xQueueReceive(displayReadyMssgBox, &displayMssg, 100);
			}
			displayMssg.line = NR_NTCS;
			if (refSensorAverager.average() / 1000.0 == ERRORTEMP)
				snprintf(line, MAXCHARSPERLINE, "ref: -");
			else
				snprintf(line, MAXCHARSPERLINE, "ref: %2.3f", refSensorAverager.average() / 1000.0);

			xQueueSend(displayMssgBox, &displayMssg, 0);
			xQueueReceive(displayReadyMssgBox, &displayMssg, 100);

			vTaskDelayUntil(&xLastWakeTime, MEASINTERVAL * 1000 / portTICK_PERIOD_MS);
		}
	}
#endif
}

// called from CGI
#ifdef ADDCGI

int getSensorNameScript(char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "Actueel,Nieuw\n");
		len += sprintf(pBuffer + len, "%s\n", userSettings.moduleName);
		return len;
		break;
	default:
		break;
	}
	return 0;
}

int getInfoValuesScript(char *pBuffer, int count) {
	int len = 0;
	char str[10];
	switch (scriptState) {
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "%s\n", "Meting,Actueel,Offset");
		for ( int n =0; n < NR_NTCS;n++) {
			sprintf (str, "Sensor %d", n+1);
			len += sprintf(pBuffer + len, "%s,%3.2f,%3.2f\n", str, lastTemperature[n]-userSettings.temperatureOffset[n], userSettings.temperatureOffset[n]); // send values and offset
		}
	//	len += sprintf(pBuffer + len, "Referentie,%3.2f,0\n", refSensorAverager.average()/1000.0);
		len += sprintf(pBuffer + len, "Referentie,0.0,0\n");//todo
		return len;
		break;
	default:
		break;
	}
	return 0;
}

// only build javascript table

int getCalValuesScript(char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "%s\n", "Meting,Referentie,Stel in,Herstel");
		len += sprintf(pBuffer + len, "%s\n", "Sensor 1\n Sensor 2\n Sensor 3\n Sensor 4\n");
		return len;
		break;
	default:
		break;
	}
	return 0;
}

int saveSettingsScript(char *pBuffer, int count) {
	saveSettings();
	return 0;
}

int cancelSettingsScript(char *pBuffer, int count) {
	loadSettings();
	return 0;
}


calValues_t calValues = { NOCAL, NOCAL, NOCAL };
// @formatter:off
char tempName[MAX_STRLEN];

const CGIdesc_t writeVarDescriptors[] = {
		{ "Temperatuur", &calValues.temperature, FLT, NR_NTCS },
		{ "moduleName",tempName, STR, 1 }
};

#define NR_CALDESCRIPTORS (sizeof (writeVarDescriptors)/ sizeof (CGIdesc_t))
// @formatter:on

int getRTMeasValuesScript(char *pBuffer, int count) {
int len = 0;

switch (scriptState) {
case 0:
	scriptState++;

	len = sprintf(pBuffer + len, "%ld,", timeStamp++);
	for (int n = 0; n < NR_NTCS; n++) {
		len += sprintf(pBuffer + len, "%3.2f,", lastTemperature[n] -userSettings.temperatureOffset[n]);
	}
#ifdef SIMULATE
	len += sprintf(pBuffer + len, "%3.3f,", lastTemperature[0] + 15);
#else
	len += sprintf(pBuffer + len, "%3.3f,",0.0);// Tmp117Temperature());
//	len += sprintf(pBuffer + len, "0.0,");  // todo
#endif
	return len;
	break;
default:
	break;
}
return 0;
}

// reads averaged values

int getAvgMeasValuesScript(char *pBuffer, int count) {
int len = 0;

switch (scriptState) {
case 0:
	scriptState++;

	len = sprintf(pBuffer + len, "%ld,", timeStamp);
	for (int n = 0; n < NR_NTCS; n++) {
		len += sprintf(pBuffer + len, "%3.2f,", (int) (logAverager[n].average() / 1000.0) - userSettings.temperatureOffset[n]);
	}
	len += sprintf(pBuffer + len, "%3.3f\n", 0.0);//  getTmp117AveragedTemperature());
		
	return len;
	break;
default:
	break;
}
return 0;

}
// these functions only work for one user!

int getNewMeasValuesScript(char *pBuffer, int count) {

int left, len = 0;
if (dayLogRxIdx != (dayLogTxIdx)) {  // something to send?
	do {
		len += sprintf(pBuffer + len, "%ld,", dayLog[dayLogRxIdx].timeStamp);
		for (int n = 0; n < NR_NTCS; n++) {
			len += sprintf(pBuffer + len, "%3.2f,", dayLog[dayLogRxIdx].temperature[n]- userSettings.temperatureOffset[n]);
		}
	//	len += sprintf(pBuffer + len, "%3.3f\n", dayLog[dayLogRxIdx].refTemperature);
		len += sprintf(pBuffer + len, "%3.3f\n", 0.0);
	//	len += sprintf(pBuffer + len, "0.0\n");  // todo
		dayLogRxIdx++;
		if (dayLogRxIdx > MAXDAYLOGVALUES)
			dayLogRxIdx = 0;
		left = count - len;

		} while ((dayLogRxIdx != dayLogTxIdx) && (left > 40));

	}
	return len;
}



// values of setcal not used, calibrate ( offset only against reference TMP117
void parseCGIWriteData(char *buf, int received) {
	if (strncmp(buf, "setCal:", 7) == 0) {  //
		float ref = (refSensorAverager.average() / 1000.0);
		for ( int n = 0; n < NR_NTCS; n++){
			if (lastTemperature[n] != ERRORTEMP ){
				float t =  logAverager[n].average() / 1000.0;
				userSettings.temperatureOffset[n] = t - ref;
			}
		}
	} else {
		if (strncmp(buf, "setName:", 8) == 0) {
			if (readActionScript(&buf[8], writeVarDescriptors, NR_CALDESCRIPTORS)) {
				if (strcmp(tempName, userSettings.moduleName) != 0) {
					strcpy(userSettings.moduleName, tempName);
					ESP_ERROR_CHECK(mdns_hostname_set(userSettings.moduleName));
					ESP_LOGI(TAG, "Hostname set to %s", userSettings.moduleName);
					saveSettings();
				}
			}
		}
	}
}

#endif
