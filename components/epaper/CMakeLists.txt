set(srcs "epdif.cpp" "epdpaint.cpp" "font8.cpp" "font12.cpp" "font16.cpp" "font20.cpp" "font24.cpp" "epd2in9_V2.cpp")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "include"
		    REQUIRES esp_common driver)
