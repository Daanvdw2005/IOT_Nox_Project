#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"  // <-- Dit toevoegen voor GPIO_NUM_ constanten

// UART-configuratie
#define UART_PORT_NUM UART_NUM_1
#define TXD_PIN (GPIO_NUM_16)
#define RXD_PIN (GPIO_NUM_17)
#define UART_BUF_SIZE (1024)

// -----------------------------
// NMEA Parser
// -----------------------------
int parse_nmea(char* sentence, float* lat, float* lon) {
    // Controleer of de zin begint met $GPGGA
    if (strncmp(sentence, "$GPGGA", 6) != 0) {
        printf("Ongeldige zin: geen $GPGGA\n");
        return 0;
    }

    // Eenvoudige checksum-validatie (basis: controleer op asterisk)
    char *asterisk = strchr(sentence, '*');
    if (asterisk == NULL) {
        printf("Geen checksum gevonden\n");
        return 0;
    }

    char *sentence_copy = strdup(sentence);
    char *token;
    int field_count = 0;
    float latitude = 0.0, longitude = 0.0;
    int valid_fix = 0;

    token = strtok(sentence_copy, ",");
    while (token != NULL) {
        if (field_count == 2) {
            // Latitude (DDMM.MMMM)
            float raw = atof(token);
            if (raw > 0) {
                int degrees = (int)(raw / 100);
                float minutes = raw - (degrees * 100);
                latitude = degrees + (minutes / 60.0);
            }
        } else if (field_count == 3) {
            if (strcmp(token, "S") == 0) latitude = -latitude;
        } else if (field_count == 4) {
            // Longitude (DDDMM.MMMM)
            float raw = atof(token);
            if (raw > 0) {
                int degrees = (int)(raw / 100);
                float minutes = raw - (degrees * 100);
                longitude = degrees + (minutes / 60.0);
            }
        } else if (field_count == 5) {
            if (strcmp(token, "W") == 0) longitude = -longitude;
        } else if (field_count == 6) {
            // Fix-kwaliteit
            if (atoi(token) > 0) {
                valid_fix = 1;
            }
        }
        token = strtok(NULL, ",");
        field_count++;
    }

    free(sentence_copy);

    if (valid_fix) {
        *lat = latitude;
        *lon = longitude;
        return 1;
    }
    return 0;
}

// -----------------------------
// UART-initialisatie
// -----------------------------
void uart_init(void) {
    uart_config_t uart_config = {
        .baud_rate = 9600, // Standaard baudrate voor veel GPS-modules
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
}

// -----------------------------
// Setup & Loop
// -----------------------------
void setup(void) {
    printf("GPS Parser ESP32 gestart\n");
    uart_init();
}

void loop(void) {
    char buffer[UART_BUF_SIZE];
    float lat = 0.0, lon = 0.0;

    // Lees UART-gegevens
    int len = uart_read_bytes(UART_PORT_NUM, (uint8_t*)buffer, UART_BUF_SIZE - 1, pdMS_TO_TICKS(1000));
    if (len > 0) {
        buffer[len] = '\0'; // Null-terminate de string
        // if (parse_nmea(buffer, &lat, &lon)) {
        //     printf("Lat: %.6f, Lon: %.6f\n", lat, lon);
        // } else {
        //     printf("Geen geldige fix of ongeldige zin\n");
        // }
        printf(buffer);
    } else {
        printf("Geen gegevens ontvangen van GPS\n");
    }

    vTaskDelay(pdMS_TO_TICKS(1000)); // Wacht 1 seconde
}

// -----------------------------
// FreeRTOS Task
// -----------------------------
void gps_task(void *pvParameters) {
    setup();
    while (1) {
        loop();
    }
}

// -----------------------------
// Entry Point ESP-IDF
// -----------------------------
void app_main(void) {
    xTaskCreate(gps_task, "gps_task", 4096, NULL, 5, NULL);
}