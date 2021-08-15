#include <string.h>
#include <stdlib.h>
#include <time.h>

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

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_tls.h"
#include "esp_crt_bundle.h"

#include "btc_image.h"
#include "eth_image.h"

#include "cJSON.h"

#define COLORED     0
#define UNCOLORED   1

#define WEB_SERVER "api.coingecko.com"
#define WEB_PORT "443"
#define WEB_URL "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd&include_24hr_change=true&include_last_updated_at=true"

static const char *TAG = "Crypto Ticker";

typedef struct {
    double price;
    double change_24h;
    double last_updated_at;
} CRYPTO_DATA;

static const char REQUEST[] = "GET " WEB_URL " HTTP/1.1\r\n"
                             "Host: " WEB_SERVER "\r\n"
                             "User-Agent: esp-idf/1.0 esp32\r\n"
                             "Accept: application/json\r\n"
                             "Connection: close\r\n"
                             "\r\n";

#ifdef __cplusplus
extern "C" {
#endif

static char* ExtractJson(char *pcBuffer) {
    //  EMPTY?
    int32_t iStrLen = strlen(pcBuffer);
    if (iStrLen <= 0) {
        ESP_LOGE(TAG, "ExtractJson failed: String is empty!");
        return NULL;
    }

    //  TRIM LEFT
    bool bFoundTrimPos = false;
    for (int32_t iLoop = 0; iLoop < iStrLen; iLoop++) {
        if (pcBuffer[iLoop] == '[' || pcBuffer[iLoop] == '{') {
            bFoundTrimPos = true;
            pcBuffer = &pcBuffer[iLoop];
            break;
        }
    }
    if (!bFoundTrimPos) {
        ESP_LOGE(TAG, "ExtractJson: TRIM LEFT FAIL!");
        return NULL;
    }
    iStrLen = strlen(pcBuffer);

    //  TRIM RIGHT
    bFoundTrimPos = false;
    for (int32_t iLoop = iStrLen - 1; iLoop >= 0; iLoop--) {
        if (pcBuffer[iLoop] == ']' || pcBuffer[iLoop] == '}') {
            bFoundTrimPos = true;
            pcBuffer[iLoop + 1] = 0; //TERMINATE
            break;
        }
    }
    if (!bFoundTrimPos) {
        ESP_LOGE(TAG, "ExtractJson: TRIM RIGHT FAIL!");
        return NULL;
    }
    iStrLen = strlen(pcBuffer);

    return pcBuffer;
}

static CRYPTO_DATA https_get_request(char *crypto_id)
{
    char buf[1024];
    int ret, len, response_size;
    char *full_response;
    size_t written_bytes = 0;
    static char *json;
    CRYPTO_DATA crypto_data;
    cJSON *root, *crypto;

    ESP_LOGI(TAG, "https_request using crt bundle");
    esp_tls_cfg_t cfg = {
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    struct esp_tls *tls = esp_tls_conn_http_new(WEB_URL, &cfg);

    if (tls != NULL) {
        ESP_LOGI(TAG, "Connection established...");
    } else {
        ESP_LOGE(TAG, "Connection failed...");
        goto exit;
    }

    do {
        ret = esp_tls_conn_write(tls,
                                 REQUEST + written_bytes,
                                 sizeof(REQUEST) - written_bytes);
        if (ret >= 0) {
            ESP_LOGI(TAG, "%d bytes written", ret);
            written_bytes += ret;
        } else if (ret != ESP_TLS_ERR_SSL_WANT_READ  && ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
            ESP_LOGE(TAG, "esp_tls_conn_write  returned: [0x%02X](%s)", ret, esp_err_to_name(ret));
            goto exit;
        }
    } while (written_bytes < sizeof(REQUEST));

    ESP_LOGI(TAG, "Reading HTTP response...");

    response_size = 2048;
    full_response = (char *) malloc(response_size);
    full_response[0] = 0;

    do {
        len = sizeof(buf) - 1;
    memset(buf, 0, sizeof(buf));
        ret = esp_tls_conn_read(tls, (char *)buf, len);

        if (ret == ESP_TLS_ERR_SSL_WANT_WRITE  || ret == ESP_TLS_ERR_SSL_WANT_READ) {
            continue;
        }

        if (ret < 0) {
            ESP_LOGE(TAG, "esp_tls_conn_read  returned [-0x%02X](%s)", -ret, esp_err_to_name(ret));
            break;
        }

        if (ret == 0) {
            ESP_LOGI(TAG, "connection closed");
            break;
        }

        len = ret;
        ESP_LOGD(TAG, "%d bytes read", len);

    while(strlen(buf) + strlen(full_response) >= response_size - 1) {
        printf("0\r\n");
        realloc(full_response, response_size + 512);
        printf("1\r\n");
        response_size += 512;
    }
    printf("2\r\n");
    strcat(full_response, buf);

    } while (1);

    json = ExtractJson(full_response);
    ESP_LOGE(TAG, "%s", json);
    root = cJSON_Parse(json);
    crypto = cJSON_GetObjectItem(root, crypto_id);
    crypto_data.price = cJSON_GetObjectItem(crypto, "usd")->valuedouble;
    crypto_data.change_24h = cJSON_GetObjectItem(crypto, "usd_24h_change")->valuedouble;
    crypto_data.last_updated_at = cJSON_GetObjectItem(crypto, "last_updated_at")->valuedouble;

    ESP_LOGI(TAG, "Value of %s: $%.2f", crypto_id, crypto_data.price);
    free(full_response);

exit:
    esp_tls_conn_delete(tls);
    return crypto_data;
}

void epaper_display_task(void *pvParameter) {
	Epd epd;
    //double usd_price;
    CRYPTO_DATA crypto_data;
    char string_usd[20];
    char string_24hr_change[20];
    char disp_usd[30];
    char disp_24hr_change[30];
    char crypto_id[20] = "bitcoin";
    char disp_usd_heading[20] = "USD PRICE:";
    char disp_24hr_change_heading[20] = "24HR CHANGE:";
	unsigned char* image = (unsigned char*)malloc(epd.width * epd.height / 8);
	Paint paint(image, 0, 0);
	//int flag = 0;

	while(1) {
		epd.Init();		// init or wakeup
        ESP_LOGI(TAG, "init done");
		epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black

        crypto_data = https_get_request(crypto_id);

        // Set Cryptocurrency logo
		epd.SetFrameMemory_Base(epd_bitmap_btc);
		//epd.SetFrameMemory_Base(epd_bitmap_eth);

        paint.SetRotate(ROTATE_90);
        
#if 0
        // Set Cryptocurrency title 
		paint.SetWidth(16);
		paint.SetHeight(200);

		paint.Clear(UNCOLORED);
		paint.DrawStringAt(0, 4, "Bitcoin", &Font16, COLORED);
		epd.SetFrameMemory(paint.GetImage(), 5, 5, paint.GetWidth(), paint.GetHeight());
#endif

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
		//vTaskDelay(10000 / portTICK_RATE_MS);
		// maybe use vTaskSuspend in the future or use deep sleep mode
        
        // Enter deep sleep mode
        esp_wifi_stop(); // disabling wifi
        ESP_LOGI(TAG, "Enabling timer wakeup, 20 seconds");
        esp_sleep_enable_timer_wakeup(20 * 1000000); // in microseconds
        rtc_gpio_isolate(GPIO_NUM_12);  // minimize current consumption
        ESP_LOGI(TAG, "Entering deep sleep");
        esp_deep_sleep_start(); 
	}
	free(image);

}

void app_main() {
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

