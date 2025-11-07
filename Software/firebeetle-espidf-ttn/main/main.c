#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "lora_radio.h"
#include "ttn.h"

void app_main(void) {
    // Initialize the LoRa radio
    if (lora_radio_init() != ESP_OK) {
        printf("LoRa radio initialization failed\n");
        return;
    }

    // Connect to The Things Network (TTN)
    if (ttn_connect() != ESP_OK) {
        printf("TTN connection failed\n");
        return;
    }

    // Main loop to send data
    while (1) {
        const char *data = "Hello, TTN!";
        if (ttn_send_data(data, strlen(data)) != ESP_OK) {
            printf("Failed to send data to TTN\n");
        } else {
            printf("Data sent to TTN: %s\n", data);
        }
        vTaskDelay(pdMS_TO_TICKS(10000)); // Send data every 10 seconds
    }
}