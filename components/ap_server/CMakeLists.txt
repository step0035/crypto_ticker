set(srcs "ap_server.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "include"
            REQUIRES json
            REQUIRES esp_http_server 
            REQUIRES nvs_flash
            )

