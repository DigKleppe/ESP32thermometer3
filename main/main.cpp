/*
. ~/ESP/esp-idf-v5.1.1//export.sh
 . /home/dig/esp/esp-idf/export.sh
 idf.py monitor -p /dev/ttyUSB2

 -s ${openocd_path}/share/openocd/scripts -f interface/ftdi/esp32_devkitj_v1.cfg -f target/esp32.cfg -c "program_esp /mnt/linuxData/projecten/git/thermostaat/SensirionSCD30/build//app.bin 0x10000 verify"

 */


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"

#include "esp_system.h"
#include "driver/gpio.h"
//#include "driver/i2c.h"
//#include "esp_ota_ops.h"
#include "esp_http_client.h"
//#include "esp_image_format.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_wifi.h"

#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "mdns.h"
#include "wifiConnect.h"
#include "settings.h"
//#include "updateSpiffsTask.h"
#include "main.h"
//#include "i2cTask.h"
#include "measureTask.h"
#include "guiTask.h"


#define LOWPOWERDELAY  				5 // minutes before going to sleepmode after powerup (no TCP anymore)
#define WAKEUP_INTERVAL				60 // seconds

static const char *TAG = "main";

esp_err_t init_spiffs(void);

extern bool settingsChanged; // from settingsScreen
uint32_t stackWm[5];
uint32_t upTime;

TaskHandle_t connectTaskh;
TaskHandle_t I2Ctaskh;
TaskHandle_t measureTaskh;

__attribute__((weak)) int getLogScript(char *pBuffer, int count) {
	return 0;
}

__attribute__((weak)) int getRTMeasValuesScript(char *pBuffer, int count) {
	return 0;
}

__attribute__((weak)) int getInfoValuesScript(char *pBuffer, int count) {
	return 0;
}



void printTaskInfo() {
	stackWm[0] = uxTaskGetStackHighWaterMark(connectTaskh);
	stackWm[1] = uxTaskGetStackHighWaterMark(measureTaskh);
	stackWm[2] = uxTaskGetStackHighWaterMark(I2Ctaskh);

	ESP_LOGI(TAG, "FreeRTOS stack connect: %4ld measure: %4ld I2C: %4ld", stackWm[0], stackWm[1], stackWm[2]);
	ESP_LOGI(TAG, "freeHeapSize %d\n", xPortGetFreeHeapSize());

	heap_caps_print_heap_info(MALLOC_CAP_DMA);
}

//socket creation failed: Too many open files in system
extern "C" {
void app_main() {
	esp_err_t err;
	displayMssg_t recDdisplayMssg;
	char line[33];
	int timeOut = 0;

	bool toggle = false;

//	char newStorageVersion[MAX_STORAGEVERSIONSIZE] = { };

	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(LED_PIN, 0);

	ESP_ERROR_CHECK(esp_event_loop_create_default());

	ESP_LOGI(TAG, "\n **************** start *****************\n");

	err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
		ESP_LOGI(TAG, "nvs flash erased");
	}
	ESP_ERROR_CHECK(err);

	ESP_ERROR_CHECK(init_spiffs());

	err = loadSettings();
	xTaskCreate(&guiTask, "guiTask", 1024 * 4, NULL, 0, NULL);
	xTaskCreate(&measureTask, "measureTask", 3500, NULL, 2, &measureTaskh);
	wifiConnect();
	while (1) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		upTime++;
		
		if ((connectStatus != IP_RECEIVED)) {
			toggle = !toggle;
			gpio_set_level(LED_PIN, toggle);
		} else {
			gpio_set_level(LED_PIN, false);

			if (wifiSettings.updated) {
				wifiSettings.updated = false;
				saveSettings();
			}
			if (settingsChanged) {
				settingsChanged = false;
				vTaskDelay(1000 / portTICK_PERIOD_MS);
				saveSettings();
			}
		//	stats_display();
		}

		//	stats_display();
		}
	}
}


