#include "epaper.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/timer.h"

#define COLORED     0
#define UNCOLORED   1

#ifdef __cplusplus
extern "C" {
#endif

void epaper_display_task(void *pvParameter) {
	Epd epd;

	unsigned char* frame_ = (unsigned char*)malloc(epd.width * epd.height / 8);

	while(1) {
		epd.Init();
		epd.ClearFrameMemory(0xFF);
		epd.DisplayFrame();

		Paint paint_(frame_, epd.width, epd.height);
		paint_.Clear(UNCOLORED);

		ESP_LOGI("EPD", "e-Paper init and clear");
		ESP_LOGI("EPD", "Cleared display");

		vTaskDelay(2000 / portTICK_RATE_MS);
		paint_.SetRotate(ROTATE_270);
		paint_.DrawStringAt((epd.width-8)/2, 4, "Welcome!", &Font24, COLORED);
		epd.SetFrameMemory(paint_.GetImage(), 0, 10, paint_.GetWidth(), paint_.GetHeight());
		epd.DisplayFrame();
		epd.Sleep();
		vTaskDelay(20000 / portTICK_RATE_MS);
	}
	free(frame_);

}

void app_main() {
	xTaskCreate(&epaper_display_task, "epaper_display_task", 2048, NULL, 5, NULL);
}

#ifdef __cplusplus
}
#endif

