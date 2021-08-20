#ifndef CRYPTO_H
#define CRYPTO_H 

#define WEB_SERVER "api.coingecko.com"
#define WEB_PORT "443"
#define WEB_URL "https://api.coingecko.com/api/v3/simple/price?ids=dogecoin%2Cbitcoin%2Cethereum%2Ccardano%2Cstellar%2Cbinancecoin%2Cchainlink%2Cuniswap%2Cpolkadot%2Ctether&vs_currencies=usd&include_24hr_change=true&include_last_updated_at=true"

typedef struct {
    double price;
    double change_24h;
    double last_updated_at;
} CRYPTO_DATA;

extern CRYPTO_DATA crypto_data_arr[10];

const char crypto_list[][20] = {
    "bitcoin",
    "ethereum",
    "cardano",
    "tether",
    "polkadot",
    "stellar",
    "dogecoin",
    "binancecoin",
    "chainlink",
    "uniswap"
};

static const char REQUEST[] = "GET " WEB_URL " HTTP/1.1\r\n"
                             "Host: " WEB_SERVER "\r\n"
                             "User-Agent: esp-idf/1.0 esp32\r\n"
                             "Accept: application/json\r\n"
                             "Connection: close\r\n"
                             "\r\n";

char* ExtractJson(char *);
void https_get_request(void);

#endif
