/*
 * guiTask.c
 *
 *  Created on: Mar 2, 2021
 *      Author: dig
 *
 *      handles screens
 */

#include "guiTask.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "lvgl.h"
#include "driver/ledc.h"
#include "settings.h"
#include <stdio.h>

#include "lvgl.h"
#include "LCD.h"

#if CONFIG_EXAMPLE_LCD_CONTROLLER_ILI9341
#include "esp_lcd_ili9341.h"
#endif
static const char *TAG = "guiTask";

// The pixel number in horizontal and vertical
#if CONFIG_EXAMPLE_LCD_CONTROLLER_ILI9341
#define EXAMPLE_LCD_H_RES 320
#define EXAMPLE_LCD_V_RES 240
#endif

/* LCD settings */
#define EXAMPLE_LCD_SPI_NUM (SPI3_HOST)
#define EXAMPLE_LCD_PIXEL_CLK_HZ (20 * 1000 * 1000)
#define EXAMPLE_LCD_CMD_BITS (8)
#define EXAMPLE_LCD_PARAM_BITS (8)
#define EXAMPLE_LCD_COLOR_SPACE (ESP_LCD_COLOR_SPACE_RGB)
#define EXAMPLE_LCD_BITS_PER_PIXEL (16)
#define EXAMPLE_LCD_DRAW_BUFF_DOUBLE (1)
#define EXAMPLE_LCD_DRAW_BUFF_HEIGHT (50)
#define EXAMPLE_LCD_BL_ON_LEVEL (0)

/* LCD pins */
#define EXAMPLE_LCD_GPIO_SCLK (GPIO_NUM_10)
#define EXAMPLE_LCD_GPIO_MOSI (GPIO_NUM_11)
#define EXAMPLE_LCD_GPIO_DC (GPIO_NUM_12)
#define EXAMPLE_LCD_GPIO_RST (GPIO_NUM_13)
#define EXAMPLE_LCD_GPIO_CS (GPIO_NUM_14)

//#define EXAMPLE_LCD_GPIO_BL (GPIO_NUM_2)

// backlight
#define LEDC_TIMER              LEDC_TIMER_1
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          GPIO_NUM_2 // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz


/* LCD IO and panel */
static esp_lcd_panel_io_handle_t lcd_io = NULL;
static esp_lcd_panel_handle_t lcd_panel = NULL;

static void initBacklight(void);

/* LVGL display and touch */
static lv_display_t *lvgl_disp = NULL;

esp_err_t app_lcd_init(void) {
	esp_err_t ret = ESP_OK;


	// /* LCD backlight */
	// gpio_config_t bk_gpio_config = {
	// 	 .pin_bit_mask = 1ULL << EXAMPLE_LCD_GPIO_BL,
	// 	 .mode = GPIO_MODE_OUTPUT,
	// 	 .intr_type = GPIO_INTR_DISABLE,
	// };
	// ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

	/* LCD initialization */
	ESP_LOGD(TAG, "Initialize SPI bus");
	const spi_bus_config_t buscfg = {
		 .mosi_io_num = EXAMPLE_LCD_GPIO_MOSI,
		 .miso_io_num = GPIO_NUM_NC,
		 .sclk_io_num = EXAMPLE_LCD_GPIO_SCLK,
		 .quadwp_io_num = GPIO_NUM_NC,
		 .quadhd_io_num = GPIO_NUM_NC,
		 .max_transfer_sz = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT * sizeof(uint16_t),
	};
	ESP_RETURN_ON_ERROR(spi_bus_initialize(EXAMPLE_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

	ESP_LOGD(TAG, "Install panel IO");
	const esp_lcd_panel_io_spi_config_t io_config = {
		 .cs_gpio_num = EXAMPLE_LCD_GPIO_CS,
		 .dc_gpio_num = EXAMPLE_LCD_GPIO_DC,
		 .spi_mode = 0,
		 .pclk_hz = EXAMPLE_LCD_PIXEL_CLK_HZ,
		 .trans_queue_depth = 10,
		 .user_ctx = NULL,
		 .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
		 .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
	};

	//	ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EXAMPLE_LCD_SPI_NUM,
	//&io_config, &lcd_io), err, TAG, "New panel IO failed");
	ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EXAMPLE_LCD_SPI_NUM, &io_config, &lcd_io);
	if (ret == ESP_OK) {

		ESP_LOGD(TAG, "Install LCD driver");
		const esp_lcd_panel_dev_config_t panel_config = {
			 .reset_gpio_num = EXAMPLE_LCD_GPIO_RST,
			// .color_space = EXAMPLE_LCD_COLOR_SPACE,
			 .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
			 .bits_per_pixel = EXAMPLE_LCD_BITS_PER_PIXEL,
		};
		// ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7789(lcd_io, &panel_config,
		// &lcd_panel), err, TAG, "New panel failed");
		ret = esp_lcd_new_panel_ili9341(lcd_io, &panel_config, &lcd_panel);
	}

	if (ret == ESP_OK) {

		// ESP_GOTO_ON_ERROR(esp_lcd_new_panel_ili9341(lcd_io, &panel_config,
		// &lcd_panel), err, TAG, "New panel failed");

		esp_lcd_panel_reset(lcd_panel);
		esp_lcd_panel_init(lcd_panel);
		esp_lcd_panel_mirror(lcd_panel, true, true);
		esp_lcd_panel_disp_on_off(lcd_panel, true);

		/* LCD backlight on */
	//	ESP_ERROR_CHECK(gpio_set_level(EXAMPLE_LCD_GPIO_BL, EXAMPLE_LCD_BL_ON_LEVEL));
		initBacklight();
		setBacklight(userSettings.backLight);
		return ret;
	}

	ESP_LOGE(TAG, "Install panel failed");
	if (lcd_panel) {
		esp_lcd_panel_del(lcd_panel);
	}
	if (lcd_io) {
		esp_lcd_panel_io_del(lcd_io);
	}
	spi_bus_free(EXAMPLE_LCD_SPI_NUM);
	return ret;
}

esp_err_t app_lvgl_init(void) {
	/* Initialize LVGL */
	const lvgl_port_cfg_t lvgl_cfg = {
		 .task_priority = 4,			/* LVGL task priority */
		 .task_stack = 4096,			/* LVGL task stack size */
		 .task_affinity = -1,		/* LVGL task pinned to core (-1 is no affinity) */
		 .task_max_sleep_ms = 500, /* Maximum sleep in LVGL task */
		 .timer_period_ms = 5		/* LVGL timer tick period in ms */
	};
	ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL port initialization failed");

	/* Add LCD screen */
	ESP_LOGD(TAG, "Add LCD screen");
	const lvgl_port_display_cfg_t disp_cfg = {.io_handle = lcd_io,
															.panel_handle = lcd_panel,
															.buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT,
															.double_buffer = EXAMPLE_LCD_DRAW_BUFF_DOUBLE,
															.hres = EXAMPLE_LCD_H_RES,
															.vres = EXAMPLE_LCD_V_RES,
															.monochrome = false,
															.rotation =
																 {
																	  .swap_xy = true,
																	  .mirror_x = false,
																	  .mirror_y = false,
																 },
#if LVGL_VERSION_MAJOR >= 9
															.color_format = LV_COLOR_FORMAT_RGB565,
#endif
															.flags = {
																 .buff_dma = true,
	//      .buff_spiram = true,
#if LVGL_VERSION_MAJOR >= 9
																 .swap_bytes = true,
#endif
															}};
	lvgl_disp = lvgl_port_add_disp(&disp_cfg);

	return ESP_OK;
}

static void initBacklight(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
   	     .speed_mode       = LEDC_MODE,
	     .duty_resolution  = LEDC_DUTY_RES,
	     .timer_num        = LEDC_TIMER,
         .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
    	.gpio_num       = LEDC_OUTPUT_IO,
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
		.intr_type      = LEDC_INTR_DISABLE,
	    .timer_sel      = LEDC_TIMER,
		.duty           = 0, // Set duty to 0%
        .hpoint         = 0,
		.flags =
		{
			.output_invert = 1,
    	}
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void setBacklight ( int percent) {
	uint32_t duty = 0;
	if ( percent <= 0)
		percent = 1;
	if ( percent > 100)
		percent = 100;
		
	if ( percent <= 100)
		duty = ((1<<13)  * percent) / 100;
	else
		duty = percent;
	if ( duty >= (1<<13))
		duty = (1<<13) -1;


	ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));
	ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}