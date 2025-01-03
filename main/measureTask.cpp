/*
 * measureTask.cpp
 *
 * Created on: Aug 9, 2021
 * Author: dig
 */

// java.lang.RuntimeException: java.util.concurrent.CompletionException:
// java.io.UncheckedIOException: java.io.IOException: Cannot run program
// "/home/dig/.espressif/tools/esp-clang/15.0.0-23786128ae/esp-clang/bin/clangd"
// (in directory "/home/dig"): error=2, No such file or directory
//  /home/dig/.espressif/tools/esp-clang/16.0.1-fe4f10a809/esp-clang/bin
#include <cstdio>
#include <ctime>
#include <string.h>

#include <math.h>
#include <sys/_timeval.h>

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
#include "stdev.h"

#define ADDCGI

#ifdef ADDCGI
#include "mdns.h"
#endif

static const char *TAG = "measureTask";

#define TIMER_BASE_CLK                                                         \
  (APB_CLK_FREQ) /*!< Frequency of the clock on the input of the timer groups  \
                  */
#define TIMER_DIVIDER (32) //  Hardware timer clock divider
#define TIMER_SCALE                                                            \
  (TIMER_BASE_CLK / TIMER_DIVIDER) // convert counter value to seconds

#define OFFSET                                                                 \
  24 // counter value  measured without capacitor (at timer divider 8)

#define MAXDELTA 0.4 // if temperature delta > this value measure again.

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
// log_t tLog[ MAXLOGVALUES];
////static log_t lastVal;
// int logTxIdx;
// int logRxIdx;

Averager firstOrderAverager[NR_NTCS]; // first order
Averager logAverager[NR_NTCS];        // second order
Averager displayAverager[NR_NTCS];

Averager refAverager(REFAVERAGES); // for reference resistor

Averager refSensorAverager(AVGERAGESAMPLES);
float lastTemperature[NR_NTCS];
uint64_t refTimerValue;
const gpio_num_t NTCpins[] = {NTC1_PIN, NTC2_PIN, NTC3_PIN, NTC4_PIN};
uint8_t err;
gptimer_handle_t gptimer = NULL;

#define NRSTDEVVALUES 20

float stdevBuffer[NRSTDEVVALUES];
int nrStdValues;

float tmpTemperature;
//  called when capacitor is discharged
static void IRAM_ATTR gpio_isr_handler(void *arg) {
  //	gptimer_stop( gptimer);
  gptimer_get_raw_count(gptimer, &timer_counter_value);
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
  irqcntr++;
  //	gpio_set_intr_type(CAP_PIN, GPIO_INTR_DISABLE);  // disable to prevent a
  //lot of irqs
  gpio_set_intr_type(COMPARATOR_PIN,
                     GPIO_INTR_DISABLE); // disable to prevent a lot of irqs
}


/*
 * A simple helper function to print the raw timer counter value
 * and the counter value converted to seconds
 */
static void inline print_timer_counter(uint64_t counter_value) {
  printf("Counter: %ld\t", (long int)counter_value);
  //	printf("Time   : %.8f s\r\n", (double) counter_value / TIMER_SCALE);
}

static void timerInit() {
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


void IRAM_ATTR measureTask(void *pvParameters) {
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
  
 // while(1)
 // 	vTaskDelay(100);

#ifdef SIMULATE
  float x = 0;

  ESP_LOGI(TAG, "Simulating sensors");
  while (1) {

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    //	ESP_LOGI(TAG, "t: %1.2f RH:%1.1f co2:%f", lastVal.temperature,
    //lastVal.hum, lastVal.co2);

    //		sprintf(str, "3:%d\n\r", (int)lastVal.co2);
    //		UDPsendMssg(UDPTXPORT, str, strlen(str));
    //
    //		sprintf(str, "S: %s t:%1.2f RH:%1.1f co2:%d", userSettings.moduleName,
    //lastVal.temperature, lastVal.hum, (int) lastVal.co2);
    //		UDPsendMssg(UDPTX2PORT, str, strlen(str));
    //
    //		ESP_LOGI(TAG, "%s %d", str, logTxIdx);
    //
    //		if (connected) {
    //			vTaskDelay(500 / portTICK_PERIOD_MS); // wait for UDP to
    //send 			sensorDataIsSend = true;
    //		}
  }

#else
  measureSemaphore = xSemaphoreCreateMutex();
  xLastWakeTime = xTaskGetTickCount();

  timerInit();
  gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));

  esp_rom_gpio_pad_select_gpio(CAP_PIN);
  esp_rom_gpio_pad_select_gpio(RREF_PIN);
  gpio_set_direction(COMPARATOR_PIN, GPIO_MODE_INPUT);
  gpio_set_direction(RREF_PIN, GPIO_MODE_INPUT);
  gpio_set_level(RREF_PIN, 0);
  esp_rom_gpio_pad_select_gpio(RREF_PIN);
  gpio_set_drive_capability(RREF_PIN, GPIO_DRIVE_CAP_3);

  gpio_install_isr_service(1 << 3);
  //	gpio_isr_handler_add(CAP_PIN, gpio_isr_handler, (void*) CAP_PIN);
  gpio_isr_handler_add(COMPARATOR_PIN, gpio_isr_handler, (void *)CAP_PIN);

  gpio_set_level(CAP_PIN, 1);

  for (int n = 0; n < NR_NTCS; n++) {
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

  while (1) {
    counts++;

    // measure reference resistor
    gpio_set_direction(CAP_PIN, GPIO_MODE_OUTPUT); // charge capacitor
    vTaskDelay(CHARGETIME);

    xQueueReceive(gpio_evt_queue, &gpio_num, 0);  // empty queue
    gpio_set_direction(CAP_PIN, GPIO_MODE_INPUT); // charge capacitor off
    //	gpio_set_intr_type(CAP_PIN, GPIO_INTR_NEGEDGE); // irq on again
    gpio_set_intr_type(COMPARATOR_PIN, GPIO_INTR_NEGEDGE); // irq on again

    gptimer_set_raw_count(gptimer, 0);
    gpio_set_direction(RREF_PIN, GPIO_MODE_OUTPUT); // set discharge resistor on
    if (xQueueReceive(
            gpio_evt_queue, &gpio_num,
            500)) { // portMAX_DELAY))
                    //		print_timer_counter(timer_counter_value);
      ;
    } else
      printf(" No Ref ");
    gpio_set_direction(RREF_PIN, GPIO_MODE_INPUT); // ref off
    //	refAverager.write(timer_counter_value - OFFSET);
    //	refTimerValue = refAverager.average();
    refTimerValue = timer_counter_value - OFFSET;

    // measure NTC
    gpio_set_direction(CAP_PIN, GPIO_MODE_OUTPUT); // charge capacitor
    vTaskDelay(CHARGETIME);

    NTCpin = NTCpins[ntc];

    xQueueReceive(gpio_evt_queue, &gpio_num, 0);  // empty queue
    gpio_set_direction(CAP_PIN, GPIO_MODE_INPUT); // charge capacitor off
    //	gpio_set_intr_type(CAP_PIN, GPIO_INTR_NEGEDGE); // irq on again
    gpio_set_intr_type(COMPARATOR_PIN, GPIO_INTR_NEGEDGE); // irq on again

    gptimer_set_raw_count(gptimer, 0);
    gpio_set_direction(NTCpin, GPIO_MODE_OUTPUT); // set discharge NTC on

    if (xQueueReceive(gpio_evt_queue, &gpio_num, 500)) {
      gpio_set_direction(NTCpin, GPIO_MODE_INPUT); // set discharge NTC off
      //	int r = (int) ( RREF * ntcAverager[ntc].average()) / (float)
      //refTimerValue; // averaged
      int r = (int)(RREF * (timer_counter_value - OFFSET)) /
              (float)refTimerValue; // real time
      //	printf("%d: %d *%4d\t",ntc, refTimerValue, r);
      printf("\t%d: r:%d %lld *%4lld ", ntc, r, refTimerValue,
             timer_counter_value - OFFSET);

      float temp = calcTemp(r);
      bool skip = false;
      if (temp > lastTemperature[ntc]) {
        if ((temp - lastTemperature[ntc] > MAXDELTA))
          skip = true;
      } else {
        if (lastTemperature[ntc] - temp > MAXDELTA)
          skip = true;
      }
      if (skip)
        //	printf("*%2.3f\t", temp);
        printf("------\t");
      else {
        //	printf("%2.3f\t", temp);
        firstOrderAverager[ntc].write((int32_t)(temp * 1000.0));
        //	displayAverager[ntc].write((int32_t)(temp * 1000.0));
        displayAverager[ntc].write(firstOrderAverager[ntc].average());
        printf("t:%2.3f ", temp);
  		printf("fo:%2.3f ", firstOrderAverager[ntc].average() / 1000.0);
        printf("%2.3f\t", displayAverager[ntc].average() / 1000.0);

        if (counts > 5) { // skip first measurements for log
          //	logAverager[ntc].write(firstOrderAverager[ntc].average());
          logAverager[ntc].write(displayAverager[ntc].average());
          //	logAverager[ntc].write((int32_t)(temp * 1000.0));
          if (ntc == 0) {
            stdevBuffer[nrStdValues++] = (float)temp;
            if (nrStdValues >= NRSTDEVVALUES)
              nrStdValues = 0;
            stdev = calculateStandardDeviation(NRSTDEVVALUES, stdevBuffer);
            printf("stdev:%2.3f\t", stdev);
          }
        }
      }
      lastTemperature[ntc] = temp;
    } else {
      printf(" No NTC ");
      gpio_set_direction(NTCpins[ntc],
                         GPIO_MODE_INPUT); // set discharge NTC off
      lastTemperature[ntc] = ERRORTEMP;
    }
    gpio_set_direction(CAP_PIN, GPIO_MODE_OUTPUT); // charge capacitor
    ntc++;
    if (ntc == NR_NTCS) {
      ntc = 0;
      printf("\n");
      refSensorAverager.write(tmpTemperature * 1000.0);
      time(&now);
      localtime_r(&now, &timeinfo); // no use in low power mode
      if (lastminute != timeinfo.tm_min) {
        lastminute = timeinfo.tm_min; // every minute
        if (logPrescaler-- == 0) {
          logPrescaler = LOGINTERVAL;

          for (int n = 0; n < NR_NTCS; n++) {
            log.temperature[n] = logAverager[n].average() / 1000.0;
          }
          //	log.refTemperature = refSensorAverager.average() / 1000.0; //
          //from I2C TMP117
          log.refTemperature = 0.0;
          addToLog(log);
        }
      }

      //				displayMssg.showTime = 0;
      //				for (int n = 0; n < NR_NTCS; n++)
      //				{
      //					displayMssg.line = n;
      //					if (lastTemperature[n] ==
      //ERRORTEMP) 						snprintf(line, MAXCHARSPERLINE, "t%d : -", n + 1); 					else
      //						//
      //snprintf(line, MAXCHARSPERLINE, "t%d : %2.2f", n + 1,
      //lastTemperature[n]); 						snprintf(line, MAXCHARSPERLINE, "t%d : %2.2f", n +
      //1, displayAverager[n].average() / 1000.0);
      //
      //					xQueueSend(displayMssgBox,
      //&displayMssg, 0); 					xQueueReceive(displayReadyMssgBox, &displayMssg, 100);
      //				}
      //				displayMssg.line = NR_NTCS;
      //				if (refSensorAverager.average() / 1000.0
      //== ERRORTEMP) 					snprintf(line, MAXCHARSPERLINE, "ref: -"); 				else
      //					snprintf(line, MAXCHARSPERLINE,
      //"ref: %2.3f", refSensorAverager.average() / 1000.0);
      //
      //				xQueueSend(displayMssgBox, &displayMssg,
      //0); 				xQueueReceive(displayReadyMssgBox, &displayMssg, 100);

      // vTaskDelayUntil(&xLastWakeTime, MEASINTERVAL * 1000 /
      // portTICK_PERIOD_MS);
    }
    //	vTaskDelayUntil(&xLastWakeTime, MEASINTERVAL * 1000 /
    //portTICK_PERIOD_MS);
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
    for (int n = 0; n < NR_NTCS; n++) {
      sprintf(str, "Sensor %d", n + 1);
      len +=
          sprintf(pBuffer + len, "%s,%3.2f,%3.2f\n", str,
                  lastTemperature[n] - userSettings.temperatureOffset[n],
                  userSettings.temperatureOffset[n]); // send values and offset
    }
    //	len += sprintf(pBuffer + len, "Referentie,%3.2f,0\n",
    //refSensorAverager.average()/1000.0);
    len += sprintf(pBuffer + len, "Referentie,0.0,0\n"); // todo
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
    len += sprintf(pBuffer + len, "%s\n",
                   "Sensor 1\n Sensor 2\n Sensor 3\n Sensor 4\n");
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

calValues_t calValues = {NOCAL, NOCAL, NOCAL};
// @formatter:off
char tempName[MAX_STRLEN];

const CGIdesc_t writeVarDescriptors[] = {
    {"Temperatuur", &calValues.temperature, FLT, NR_NTCS},
    {"moduleName", tempName, STR, 1}};

#define NR_CALDESCRIPTORS (sizeof(writeVarDescriptors) / sizeof(CGIdesc_t))
// @formatter:on

int getRTMeasValuesScript(char *pBuffer, int count) {
  int len = 0;

  switch (scriptState) {
  case 0:
    scriptState++;

    len = sprintf(pBuffer + len, "%u,", (unsigned int)timeStamp++);
    for (int n = 0; n < NR_NTCS; n++) {
      len += sprintf(pBuffer + len, "%3.2f,",
                     lastTemperature[n] - userSettings.temperatureOffset[n]);
    }
#ifdef SIMULATE
    len += sprintf(pBuffer + len, "%3.3f,", lastTemperature[0] + 15);
#else
    len += sprintf(pBuffer + len, "%3.3f,", 0.0); // Tmp117Temperature());
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
      len += sprintf(pBuffer + len, "%3.2f,",
                     (int)(logAverager[n].average() / 1000.0) -
                         userSettings.temperatureOffset[n]);
    }
    //	len += sprintf(pBuffer + len, "%3.3f\n", 0.0); //
    //getTmp117AveragedTemperature());
    len += sprintf(pBuffer + len, "\n");

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
  if (dayLogRxIdx != (dayLogTxIdx)) { // something to send?
    do {
      len += sprintf(pBuffer + len, "%d,",(int) dayLog[dayLogRxIdx].timeStamp);
      for (int n = 0; n < NR_NTCS; n++) {
        len += sprintf(pBuffer + len, "%3.2f,",
                       dayLog[dayLogRxIdx].temperature[n] -
                           userSettings.temperatureOffset[n]);
      }
      //	len += sprintf(pBuffer + len, "%3.3f\n",
      //dayLog[dayLogRxIdx].refTemperature);
      //	len += sprintf(pBuffer + len, "%3.3f\n", 0.0);
      len += sprintf(pBuffer + len, "\n");

      //	len += sprintf(pBuffer + len, "0.0\n");  // todo
      dayLogRxIdx++;
      if (dayLogRxIdx > MAXDAYLOGVALUES)
        dayLogRxIdx = 0;
      left = count - len;

    } while ((dayLogRxIdx != dayLogTxIdx) && (left > 40));
  }
  return len;
}

void parseCGIWriteData(char *buf, int received) {
  if (strncmp(buf, "setCal:", 7) == 0) { //

    //		float ref = (refSensorAverager.average() / 1000.0);
    float ref;
    sscanf((const char *)buf, "%f", &ref);

    for (int n = 0; n < NR_NTCS; n++) {
      if (lastTemperature[n] != ERRORTEMP) {
        float t = logAverager[n].average() / 1000.0;
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
