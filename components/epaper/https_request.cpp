#include <https_request.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"

static const char *TAG = "https request";

char* ExtractJson(char *pcBuffer) {
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

CRYPTO_DATA https_get_request(char *crypto_id)
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


