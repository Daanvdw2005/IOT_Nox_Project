#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "GPS_EXAMPLE";

#define GPS_UART_NUM UART_NUM_2
#define GPS_RX_PIN 16  // GPIO16 voor RX
#define GPS_TX_PIN UART_PIN_NO_CHANGE  // Geen TX nodig
#define GPS_BAUD_RATE 9600
#define RX_BUF_SIZE 2048  // Verhoogde buffer

// Functie om $GPGGA te parsen
static void parse_gpgga(const char *sentence) {
    char copy[RX_BUF_SIZE];
    strncpy(copy, sentence, RX_BUF_SIZE - 1);
    copy[RX_BUF_SIZE - 1] = '\0';
    char *token = strtok(copy, ",");

    if (token == NULL || strcmp(token, "$GPGGA") != 0) {
        ESP_LOGW(TAG, "Geen geldige $GPGGA zin: %s", sentence);
        return;
    }

    token = strtok(NULL, ","); if (token) ESP_LOGI(TAG, "UTC Tijd: %s", token);
    token = strtok(NULL, ","); if (token) {
        double lat = atof(token); int lat_deg = (int)(lat / 100);
        double lat_min = lat - (lat_deg * 100); double latitude = lat_deg + (lat_min / 60.0);
        token = strtok(NULL, ","); if (token && *token == 'S') latitude = -latitude;
        ESP_LOGI(TAG, "Latitude: %.6f graden", latitude);
    }
    token = strtok(NULL, ","); if (token) {
        double lon = atof(token); int lon_deg = (int)(lon / 100);
        double lon_min = lon - (lon_deg * 100); double longitude = lon_deg + (lon_min / 60.0);
        token = strtok(NULL, ","); if (token && *token == 'W') longitude = -longitude;
        ESP_LOGI(TAG, "Longitude: %.6f graden", longitude);
    }
    token = strtok(NULL, ","); int fix = token ? atoi(token) : 0;
    ESP_LOGI(TAG, "Fix quality: %d (%s)", fix, fix > 0 ? "Geldig" : "Ongeldig");
    token = strtok(NULL, ","); int sats = token ? atoi(token) : 0;
    ESP_LOGI(TAG, "Satellieten: %d", sats);
    token = strtok(NULL, ","); token = strtok(NULL, ","); double altitude = token ? atof(token) : 0.0;
    ESP_LOGI(TAG, "Altitude: %.2f m", altitude);
    ESP_LOGI(TAG, "-------------------------");
}

static void gps_rx_task(void *arg) {
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    char line[RX_BUF_SIZE] = {0};
    int line_pos = 0;
    TickType_t last_log_time = 0;

    ESP_LOGI(TAG, "GPS RX taak gestart");
    while (1) {
        const int rx_bytes = uart_read_bytes(GPS_UART_NUM, data, RX_BUF_SIZE, pdMS_TO_TICKS(100));
        if (rx_bytes > 0) {
            ESP_LOGI(TAG, "Ontvangen %d bytes", rx_bytes);
            for (int i = 0; i < rx_bytes; i++) {
                if (data[i] == '\n' || data[i] == '\r') {
                    if (line_pos > 0) {
                        line[line_pos] = '\0';
                        ESP_LOGI(TAG, "Geparse regel: %s", line);
                        if (strstr(line, "$GPGGA") == line) {
                            if (xTaskGetTickCount() - last_log_time >= pdMS_TO_TICKS(1000)) {
                                parse_gpgga(line);
                                last_log_time = xTaskGetTickCount();
                            }
                        } else if (strstr(line, "$GPGSV") == line) {
                            ESP_LOGI(TAG, "Satelliet info: %s", line); // Extra debug voor satellieten
                        }
                        line_pos = 0;
                    }
                } else if (line_pos < RX_BUF_SIZE - 1) {
                    line[line_pos++] = data[i];
                } else {
                    ESP_LOGW(TAG, "Lijnbuffer overflow, regel verworpen: %s", line);
                    line_pos = 0;
                }
            }
        } else {
            ESP_LOGD(TAG, "Geen data ontvangen in afgelopen 100ms");
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    free(data);
    vTaskDelete(NULL);
}

void app_main(void) {
    const uart_config_t uart_config = {
        .baud_rate = GPS_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(GPS_UART_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(GPS_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "GPS UART geïnitialiseerd op baud %d, RX pin %d", GPS_BAUD_RATE, GPS_RX_PIN);

    xTaskCreate(gps_rx_task, "gps_rx_task", 4096, NULL, 10, NULL);
}