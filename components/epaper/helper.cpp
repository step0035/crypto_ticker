#include "helper.h"
#include "epaper.h"
#include "esp_log.h"
#include <time.h>

#include "binancecoin_image.h"
#include "btc_image.h"
#include "cardano_image.h"
#include "chainlink_image.h"
#include "dogecoin_image.h"
#include "eth_image.h"
#include "polkadot_image.h"
#include "stellar_image.h"
#include "tether_image.h"
#include "uniswap_image.h"

#define COLORED     0
#define UNCOLORED   1

static const char *TAG = "Crypto Ticker";

Epd epd;
char string_usd[20];
char string_24hr_change[20];
char disp_usd[30];
char disp_24hr_change[30];
char disp_usd_heading[20] = "USD PRICE:";
char disp_24hr_change_heading[20] = "24HR CHANGE:";
unsigned char* image = (unsigned char*)malloc(epd.width * epd.height / 8);
Paint paint(image, 0, 0);
time_t time_updated;
struct tm ts;
char time_buf[40];
char disp_time[80] = "Updated: ";

void Init(void) {
    epd.Init();
    epd.ClearFrameMemory(0xFF);
}

void SetOrientation(void) {
    paint.SetRotate(ROTATE_90);
}

void SetBackground(int wakeup_index) {
    switch (wakeup_index) {
        case 0:
            epd.SetFrameMemory_Base(epd_bitmap_btc);
            break;
        case 1:
            epd.SetFrameMemory_Base(epd_bitmap_eth);
            break;
        case 2:
            epd.SetFrameMemory_Base(epd_bitmap_cardano);
            break;
        case 3:
            epd.SetFrameMemory_Base(epd_bitmap_tether);
            break;
        case 4:
            epd.SetFrameMemory_Base(epd_bitmap_polkadot);
            break;
        case 5:
            epd.SetFrameMemory_Base(epd_bitmap_stellar);
            break;
        case 6:
            epd.SetFrameMemory_Base(epd_bitmap_dogecoin);
            break;
        case 7:
            epd.SetFrameMemory_Base(epd_bitmap_binancecoin);
            break;
        case 8:
            epd.SetFrameMemory_Base(epd_bitmap_chainlink);
            break;
        case 9:
            epd.SetFrameMemory_Base(epd_bitmap_uniswap);
            break;
        default:
            break;
    }
}

void SetPriceHeading(void) {
		paint.SetWidth(18);
		paint.SetHeight(160);

		paint.Clear(UNCOLORED);
		paint.DrawStringAt(0, 4, disp_usd_heading, &Font16, COLORED);
		epd.SetFrameMemory(paint.GetImage(), 86, 120, paint.GetWidth(), paint.GetHeight());
}

void SetPrice(CRYPTO_DATA crypto_data) {
        sprintf(string_usd, "%.2f", crypto_data.price);
        sprintf(disp_usd, "$");
        strcat(disp_usd, string_usd);

		paint.SetWidth(24);
		paint.SetHeight(200);

		paint.Clear(UNCOLORED);
		paint.DrawStringAt(0, 4, disp_usd, &Font24, COLORED);
		epd.SetFrameMemory(paint.GetImage(), 64, 120, paint.GetWidth(), paint.GetHeight());
}

void Set24hHeading(void) {
		paint.SetWidth(18);
		paint.SetHeight(160);

		paint.Clear(UNCOLORED);
		paint.DrawStringAt(0, 4, disp_24hr_change_heading, &Font16, COLORED);
		epd.SetFrameMemory(paint.GetImage(), 31, 120, paint.GetWidth(), paint.GetHeight());
}

void Set24hChange(CRYPTO_DATA crypto_data) {
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
}

void SetLastUpdated(CRYPTO_DATA crypto_data) {
        memset(disp_time, 0, sizeof(disp_time));
        sprintf(disp_time, "Updated: ");
        time_updated = crypto_data.last_updated_at + (60 * 60 * 8); // GMT +8
        ts = *localtime(&time_updated);
        strftime(time_buf, sizeof(time_buf), "%a %Y-%m-%d %H:%M:%S", &ts);
        ESP_LOGW(TAG, "%s", time_buf);
        strcat(disp_time, time_buf);

		paint.SetWidth(14);
		paint.SetHeight(250);

		paint.Clear(UNCOLORED);
		paint.DrawStringAt(0, 4, disp_time, &Font12, COLORED);
		epd.SetFrameMemory(paint.GetImage(), 114, 5, paint.GetWidth(), paint.GetHeight());
}

void UpdateDisplay(void) {
		epd.DisplayFrame();
        memset(disp_usd, 0, sizeof(disp_usd));
        memset(disp_24hr_change, 0, sizeof(disp_24hr_change));
}

void DeInit(void) {
        free(image);
}
