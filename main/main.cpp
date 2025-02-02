#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_freertos_hooks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "mdns.h"
#include "settings.h"
#include "spiffs.h"
#include "wifiConnect.h"

#include "guiTask.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "main";

const gpio_num_t LED_PIN = GPIO_NUM_9;

uint32_t timeStamp;

void vTimerCallback(TimerHandle_t xTimer)
{
	timeStamp++;
}

extern "C"
{
	void app_main()
	{
		esp_err_t err;
		displayMssg_t displayMssg;
		char line[50];
		int timeOut = 0;
		uint32_t upTime = 0;
		bool toggle = false;
		TimerHandle_t xTimer;

		//	char newStorageVersion[MAX_STORAGEVERSIONSIZE] = { };

		gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
		gpio_set_level(LED_PIN, 0);

		ESP_ERROR_CHECK(esp_event_loop_create_default());

		ESP_LOGI(TAG, "\n **************** start *****************\n");

		err = nvs_flash_init();
		if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
		{
			ESP_ERROR_CHECK(nvs_flash_erase());
			err = nvs_flash_init();
			ESP_LOGI(TAG, "nvs flash erased");
		}
		ESP_ERROR_CHECK(err);

		ESP_ERROR_CHECK(init_spiffs());

		err = loadSettings();
		ESP_LOGI(TAG, "connecting to %s", wifiSettings.SSID);

		xTimer = xTimerCreate("Timer", 10, pdTRUE, (void *)0, vTimerCallback);
		xTimerStart(xTimer, 0);

		xTaskCreate(&guiTask, "guiTask", 1024 * 8, NULL, 0, NULL);
		xTaskCreatePinnedToCore(&measureTask, "measureTask", 3500, NULL, 2, NULL,1);
		wifiConnect();

		displayMssg.line = 5;
		displayMssg.str1 = line;
		displayMssg.displayItem = DISPLAY_ITEM_MEASLINE;
		displayMssg.showTime = 0;

		while (1)
		{
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			upTime++;
			if (settingsChanged)
			{
				settingsChanged = false;
				saveSettings();
			}
			if ((connectStatus != IP_RECEIVED))
			{
				toggle = !toggle;
				gpio_set_level(LED_PIN, toggle);
				sprintf(line, "Verbinden met %s", wifiSettings.SSID);
				xQueueSend(displayMssgBox, &displayMssg, 0);
			}
			else
			{
				gpio_set_level(LED_PIN, false);

				if (wifiSettings.updated)
				{
					wifiSettings.updated = false;
					saveSettings();
				}
				if (settingsChanged)
				{
					settingsChanged = false;
					vTaskDelay(1000 / portTICK_PERIOD_MS);
					saveSettings();
				}

				sprintf(line, "%s  %s", wifiSettings.SSID, myIpAddress);
				xQueueSend(displayMssgBox, &displayMssg, 0);

				//	stats_display();
			}

			//	stats_display();
		}
	}
}