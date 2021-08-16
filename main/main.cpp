#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "https_request.h"

#include "epaper.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "driver/timer.h"
#include "driver/rtc_io.h"

#include "btc_image.h"
#include "eth_image.h"

#define COLORED     0
#define UNCOLORED   1

static const char *TAG = "Crypto Ticker";
static RTC_DATA_ATTR int wakeup_index = 0;

#ifdef __cplusplus
extern "C" {
#endif

void epaper_display_task(void *pvParameter) {
	Epd epd;
    //double usd_price;
    CRYPTO_DATA crypto_data;
    char string_usd[20];
    char string_24hr_change[20];
    char disp_usd[30];
    char disp_24hr_change[30];
    //char crypto_id[20] = "bitcoin";
    char disp_usd_heading[20] = "USD PRICE:";
    char disp_24hr_change_heading[20] = "24HR CHANGE:";
	unsigned char* image = (unsigned char*)malloc(epd.width * epd.height / 8);
	Paint paint(image, 0, 0);
	//int flag = 0;

	while(1) {
		epd.Init();		// init or wakeup
        ESP_LOGI(TAG, "init done");
		epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black

        https_get_request();
        crypto_data = crypto_data_arr[wakeup_index];

        // Set Cryptocurrency logo
		epd.SetFrameMemory_Base(epd_bitmap_btc);
		//epd.SetFrameMemory_Base(epd_bitmap_eth);

        paint.SetRotate(ROTATE_90);
        
        // Set Cryptocurrency price heading
		paint.SetWidth(18);
		paint.SetHeight(160);

		paint.Clear(UNCOLORED);
		paint.DrawStringAt(0, 4, disp_usd_heading, &Font16, COLORED);
		epd.SetFrameMemory(paint.GetImage(), 86, 120, paint.GetWidth(), paint.GetHeight());

        // Set Cryptocurrency price
        sprintf(string_usd, "%.2f", crypto_data.price);
        sprintf(disp_usd, "$");
        strcat(disp_usd, string_usd);

		paint.SetWidth(24);
		paint.SetHeight(200);

		paint.Clear(UNCOLORED);
		paint.DrawStringAt(0, 4, disp_usd, &Font24, COLORED);
		epd.SetFrameMemory(paint.GetImage(), 64, 120, paint.GetWidth(), paint.GetHeight());

        // Set 24hr change heading
		paint.SetWidth(18);
		paint.SetHeight(160);

		paint.Clear(UNCOLORED);
		paint.DrawStringAt(0, 4, disp_24hr_change_heading, &Font16, COLORED);
		epd.SetFrameMemory(paint.GetImage(), 31, 120, paint.GetWidth(), paint.GetHeight());

        // Set 24hr change 
        sprintf(string_24hr_change, "%.2f", crypto_data.change_24h);
        if((double) crypto_data.change_24h > 0)
            sprintf(disp_24hr_change, "+");
        else 
            disp_24hr_change[0] = 0;
        strcat(disp_24hr_change, string_24hr_change);
        strcat(disp_24hr_change, "%");
        
		paint.SetWidth(24);
		paint.SetHeight(200);

		paint.Clear(UNCOLORED);
		paint.DrawStringAt(0, 4, disp_24hr_change, &Font24, COLORED);
		epd.SetFrameMemory(paint.GetImage(), 9, 120, paint.GetWidth(), paint.GetHeight());

        // Set Last updated 
        time_t time = crypto_data.last_updated_at + (60 * 60 * 8); // GMT +8
        struct tm ts;
        char time_buf[40];
        char disp_time[80] = "Updated: ";
        ts = *localtime(&time);
        strftime(time_buf, sizeof(time_buf), "%a %Y-%m-%d %H:%M:%S", &ts);
        ESP_LOGW(TAG, "%s", time_buf);
        strcat(disp_time, time_buf);

		paint.SetWidth(14);
		paint.SetHeight(250);

		paint.Clear(UNCOLORED);
		paint.DrawStringAt(0, 4, disp_time, &Font12, COLORED);
		epd.SetFrameMemory(paint.GetImage(), 114, 5, paint.GetWidth(), paint.GetHeight());

        // Update the display
		epd.DisplayFrame();
        memset(disp_usd, 0, sizeof(disp_usd));
        memset(disp_24hr_change, 0, sizeof(disp_24hr_change));
        memset(disp_time, 0, sizeof(disp_time));
        ESP_LOGI(TAG, "Going to sleep...");
		//epd.Sleep();
        
        // Enter deep sleep mode
        ESP_LOGI(TAG, "Disabling wifi to prepare for deep sleep");
        esp_wifi_stop(); // disabling wifi
        ESP_LOGI(TAG, "Enabling timer wakeup, 20 seconds");
        esp_sleep_enable_timer_wakeup(20 * 1000000); // enable wakeup by timer in microseconds
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1);   // enable wakeup by gpio HIGH
        rtc_gpio_isolate(GPIO_NUM_12);  // minimize current consumption
        ESP_LOGI(TAG, "Entering deep sleep");
        esp_deep_sleep_start(); 
	}
	free(image);

}

void app_main() {
    int wakeup_cause;
    wakeup_cause = esp_sleep_get_wakeup_cause();    // Check if wakeup caused by rtc_gpio interrupt 
    ESP_LOGW(TAG, "wakeup_cause: %d", wakeup_cause);

    if (wakeup_cause == ESP_SLEEP_WAKEUP_EXT0) {
        wakeup_index++;
    }
    ESP_LOGW(TAG, "wakeup_index: %d", wakeup_index);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Helper function to configure wifi or ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    // Tasks to schedule
	xTaskCreate(&epaper_display_task, "epaper_display_task", 8192, NULL, 10, NULL);
}

#ifdef __cplusplus
}
#endif

