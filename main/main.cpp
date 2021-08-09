#include "epaper.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/timer.h"
//#include "imagedata.h"
#include "testimage.h"

#define COLORED     0
#define UNCOLORED   1

#ifdef __cplusplus
extern "C" {
#endif

void epaper_display_task(void *pvParameter) {
	Epd epd;
	unsigned char* image = (unsigned char*)malloc(epd.width * epd.height / 8);
	Paint paint(image, 0, 0);
	int flag = 0;

	while(1) {
		epd.Init(flag);
		epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
		epd.DisplayFrame();
		vTaskDelay(1000 / portTICK_RATE_MS);

		paint.SetRotate(ROTATE_0);
		paint.SetWidth(128);
		paint.SetHeight(24);

		/* For simplicity, the arguments are explicit numerical coordinates */
		paint.Clear(COLORED);
		paint.DrawStringAt(0, 4, "Hello world!", &Font12, UNCOLORED);
		epd.SetFrameMemory(paint.GetImage(), 0, 10, paint.GetWidth(), paint.GetHeight());

		paint.Clear(UNCOLORED);
		paint.DrawStringAt(0, 4, "e-Paper Demo", &Font12, COLORED);
		epd.SetFrameMemory(paint.GetImage(), 0, 30, paint.GetWidth(), paint.GetHeight());

		paint.SetWidth(64);
		paint.SetHeight(64);

		paint.Clear(UNCOLORED);
		paint.DrawRectangle(0, 0, 40, 50, COLORED);
		paint.DrawLine(0, 0, 40, 50, COLORED);
		paint.DrawLine(40, 0, 0, 50, COLORED);
		epd.SetFrameMemory(paint.GetImage(), 8, 60, paint.GetWidth(), paint.GetHeight());

		paint.Clear(UNCOLORED);
		paint.DrawCircle(32, 32, 30, COLORED);
		epd.SetFrameMemory(paint.GetImage(), 56, 60, paint.GetWidth(), paint.GetHeight());

		paint.Clear(UNCOLORED);
		paint.DrawFilledRectangle(0, 0, 40, 50, COLORED);
		epd.SetFrameMemory(paint.GetImage(), 8, 130, paint.GetWidth(), paint.GetHeight());

		paint.Clear(UNCOLORED);
		paint.DrawFilledCircle(32, 32, 30, COLORED);
		epd.SetFrameMemory(paint.GetImage(), 56, 130, paint.GetWidth(), paint.GetHeight());
		epd.DisplayFrame();

		vTaskDelay(5000 / portTICK_RATE_MS);
		//epd.SetFrameMemory_Base(IMAGE_DATA_TEST);
		epd.SetFrameMemory(IMAGE_DATA_TEST);
		epd.DisplayFrame();

		epd.Sleep();
		flag = 1;
		vTaskDelay(10000 / portTICK_RATE_MS);
	}
	free(image);

}

void app_main() {
	xTaskCreate(&epaper_display_task, "epaper_display_task", 2048, NULL, 5, NULL);
}

#ifdef __cplusplus
}
#endif

