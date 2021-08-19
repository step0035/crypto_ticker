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

TaskHandle_t epdTaskHandle;
TaskHandle_t intrTaskHandle;

SemaphoreHandle_t xSemaphore_intr = NULL;

static const char *TAG = "Crypto Ticker";
static volatile int list_id = 0;
CRYPTO_DATA crypto_data = {0};

#ifdef __cplusplus
extern "C" {
#endif

void epaper_display_task(void *pvParameter) {
	while(1) {
        ESP_LOGI(TAG, "epaper_display_task");
        vTaskSuspend(intrTaskHandle);
        https_get_request();

        crypto_data = crypto_data_arr[list_id];

        SetOrientation();
        SetBackground(list_id);
        SetPriceHeading();
        SetPrice(crypto_data);
        Set24hHeading();
        Set24hChange(crypto_data);
        SetLastUpdated(crypto_data);
        UpdateDisplay();
        ESP_LOGI(TAG, "Updated EPD");

        vTaskResume(intrTaskHandle);

        ESP_LOGI(TAG, "Delay");
        vTaskDelay(20000 / portTICK_RATE_MS);
	}
    DeInit();
}

void intr_update_display(void *args) {
    while(1) {
        if (xSemaphoreTake(xSemaphore_intr, (TickType_t) 10)) {
            ESP_LOGI("Button Interrupt", "Button interrupt handler");
            list_id++;
            list_id %= 10;
            crypto_data = crypto_data_arr[list_id];

            SetOrientation();
            SetBackground(list_id);
            SetPriceHeading();
            SetPrice(crypto_data);
            Set24hHeading();
            Set24hChange(crypto_data);
            SetLastUpdated(crypto_data);
            UpdateDisplay();
            ESP_LOGI(TAG, "Updated EPD from button interrupt");
        }
    }
}

static void gpio_button_isr_handler(void* arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    gpio_intr_disable(GPIO_NUM_33);
    //vTaskDelay(20 / portTICK_RATE_MS);
    xSemaphoreGiveFromISR(xSemaphore_intr, &xHigherPriorityTaskWoken);
    gpio_intr_enable(GPIO_NUM_33);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void app_main() {
    Init();
    xSemaphore_intr = xSemaphoreCreateBinary();

    // Initialize interrupt gpio
    gpio_config_t intr_conf = {0};
    intr_conf.intr_type = GPIO_INTR_POSEDGE;
    intr_conf.mode = GPIO_MODE_INPUT;
    intr_conf.pin_bit_mask = (1ULL<<GPIO_NUM_33);
    intr_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&intr_conf));

    //gpio_isr_register(gpio_button_isr_handler, NULL, ESP_INTR_FLAG_LEVEL1, NULL);
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    gpio_isr_handler_add(GPIO_NUM_33, gpio_button_isr_handler, NULL);

    ESP_LOGW(TAG, "list_id: %d", list_id);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Helper function to configure wifi or ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    // Tasks to schedule
	xTaskCreate(epaper_display_task, "epaper_display_task", 8192, NULL, 7, &epdTaskHandle);
	xTaskCreate(intr_update_display, "interrupt update display", 2048, NULL, 9, &intrTaskHandle);
}

#ifdef __cplusplus
}
#endif

