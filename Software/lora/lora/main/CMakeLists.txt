idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES ttn-esp32 nvs_flash driver esp_adc)