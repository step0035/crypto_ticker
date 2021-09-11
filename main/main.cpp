#include <string.h>
#include <stdlib.h>

#include "crypto.h"
#include "helper.h"
#include "epaper.h"
#include "ap_server.h"

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
//#include "protocol_examples_common.h"
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
        //vTaskSuspend(intrTaskHandle);
        gpio_intr_disable(GPIO_NUM_33);
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

        //vTaskResume(intrTaskHandle);
        gpio_intr_enable(GPIO_NUM_33);

        ESP_LOGI(TAG, "Delay");
        vTaskDelay(5 * 60 * 1000 / portTICK_RATE_MS);
        //vTaskDelay( 20000 / portTICK_RATE_MS);
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
    gpio_intr_disable(GPIO_NUM_33);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphore_intr, &xHigherPriorityTaskWoken);
    gpio_intr_enable(GPIO_NUM_33);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void cb_connection_ok(void *args) { // delete this later, need reference for task suspension/resume
    ip_event_got_ip_t *param = (ip_event_got_ip_t*)args;
    char str_ip[16];
    esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);
    ESP_LOGI(TAG, "Connection established, IP: %s", str_ip);
    vTaskDelay(5000 / portTICK_RATE_MS);
    vTaskResume(epdTaskHandle);
    vTaskResume(intrTaskHandle);
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

    // Start softAP server if not connected to wifi AP
    ap_server_start();

    // Tasks to schedule
	xTaskCreate(epaper_display_task, "epaper_display_task", 8192, NULL, 7, &epdTaskHandle);
	xTaskCreate(intr_update_display, "interrupt update display", 2048, NULL, 9, &intrTaskHandle);
    vTaskSuspend(epdTaskHandle);
    vTaskSuspend(intrTaskHandle);
}

#ifdef __cplusplus
}
#endif

