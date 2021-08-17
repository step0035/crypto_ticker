#include <string.h>
#include <stdlib.h>

#include "crypto.h"
#include "helper.h"
#include "epaper.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "driver/rtc_io.h"

#include "btc_image.h"
#include "eth_image.h"

static const char *TAG = "Crypto Ticker";
static volatile RTC_DATA_ATTR int wakeup_index = 0;
CRYPTO_DATA crypto_data;
SemaphoreHandle_t xSemaphore = NULL;

#ifdef __cplusplus
extern "C" {
#endif

void epaper_display_task(void *pvParameter) {
	while(1) {
        https_get_request();
        crypto_data = crypto_data_arr[wakeup_index];

        SetOrientation();
        SetBackground(wakeup_index);
        SetPriceHeading();
        SetPrice(crypto_data);
        Set24hHeading();
        Set24hChange(crypto_data);
        SetLastUpdated(crypto_data);
        UpdateDisplay();

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
    DeInit();
}

void intr_update_display(void *args) {
    while(1) {
        if (xSemaphoreTake(xSemaphore, (TickType_t) 10)) {
            wakeup_index++;
            wakeup_index %= 10;
            ESP_LOGI("Button Interrupt", "Button interrupt handler");
            crypto_data = crypto_data_arr[wakeup_index];

            SetOrientation();
            SetBackground(wakeup_index);
            SetPriceHeading();
            SetPrice(crypto_data);
            Set24hHeading();
            Set24hChange(crypto_data);
            SetLastUpdated(crypto_data);
            UpdateDisplay();
        }
    }
}

static void IRAM_ATTR gpio_button_isr_handler(void* arg) {
    gpio_intr_disable(GPIO_NUM_33);
    //vTaskDelay(20 / portTICK_RATE_MS);
    xSemaphoreGiveFromISR(xSemaphore, NULL);
    gpio_intr_enable(GPIO_NUM_33);
}

void app_main() {
    Init();
    xSemaphore = xSemaphoreCreateBinary();
    // Initialize interrupt gpio
    ESP_LOGI(TAG, "TEST1");
    gpio_config_t intr_conf = {0};
    ESP_LOGI(TAG, "TEST2");
    intr_conf.intr_type = GPIO_INTR_POSEDGE;
    ESP_LOGI(TAG, "TEST3");
    intr_conf.mode = GPIO_MODE_INPUT;
    ESP_LOGI(TAG, "TEST4");
    intr_conf.pin_bit_mask = (1ULL<<GPIO_NUM_33);
    ESP_LOGI(TAG, "TEST5");
    //intr_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    ESP_LOGI(TAG, "TEST6");
    ESP_ERROR_CHECK(gpio_config(&intr_conf));
    ESP_LOGI(TAG, "TEST7");
    gpio_isr_register(gpio_button_isr_handler, NULL, ESP_INTR_FLAG_LEVEL1, NULL);
#if 0
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    ESP_LOGI(TAG, "TEST8");
    gpio_isr_handler_add(GPIO_NUM_33, gpio_button_isr_handler, NULL);
    ESP_LOGI(TAG, "TEST9");
#endif

    int wakeup_cause;
    wakeup_cause = esp_sleep_get_wakeup_cause();    // Check if wakeup caused by rtc_gpio interrupt 
    ESP_LOGW(TAG, "wakeup_cause: %d", wakeup_cause);

    if (wakeup_cause == ESP_SLEEP_WAKEUP_EXT0) {
        wakeup_index++;
        wakeup_index %= 10;
        crypto_data = crypto_data_arr[wakeup_index];

        SetOrientation();
        SetBackground(wakeup_index);
        SetPriceHeading();
        SetPrice(crypto_data);
        Set24hHeading();
        Set24hChange(crypto_data);
        SetLastUpdated(crypto_data);
        UpdateDisplay();

        esp_sleep_enable_timer_wakeup(20 * 1000000); // enable wakeup by timer in microseconds
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1);   // enable wakeup by gpio HIGH
        rtc_gpio_isolate(GPIO_NUM_12);  // minimize current consumption
        ESP_LOGI(TAG, "Entering deep sleep");
        esp_deep_sleep_start(); 
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
	xTaskCreate(&epaper_display_task, "epaper_display_task", 8192, NULL, 5, NULL);
	xTaskCreatePinnedToCore(&intr_update_display, "interrupt update display", 2048, NULL, 10, NULL, 1);
}

#ifdef __cplusplus
}
#endif

