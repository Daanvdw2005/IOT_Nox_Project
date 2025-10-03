#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// -----------------------------
// NMEA Parser
// -----------------------------
int parse_nmea(char* sentence, float* lat, float* lon) {
    char *token;
    char* sentence_copy = strdup(sentence); // Kopie maken voor strtok
    int field_count = 0;
    float latitude = 0.0, longitude = 0.0;
    int valid_fix = 0;

    token = strtok(sentence_copy, ",");
    while (token != NULL) {
        if (field_count == 2) { 
            // Latitude (DDMM.MMMM)
            float raw = atof(token);
            int degrees = (int)(raw / 100);
            float minutes = raw - (degrees * 100);
            latitude = degrees + (minutes / 60.0);
        } 
        else if (field_count == 3) { 
            if (strcmp(token, "S") == 0) latitude = -latitude;
        } 
        else if (field_count == 4) { 
            // Longitude (DDDMM.MMMM)
            float raw = atof(token);
            int degrees = (int)(raw / 100);
            float minutes = raw - (degrees * 100);
            longitude = degrees + (minutes / 60.0);
        } 
        else if (field_count == 5) { 
            if (strcmp(token, "W") == 0) longitude = -longitude;
        } 
        else if (field_count == 6) { 
            // Fix quality
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
// Demo GPS zin (voor test)
// -----------------------------
const char* test_sentence = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";

// -----------------------------
// Setup & Loop
// -----------------------------
void setup(void) {
    printf("GPS Parser ESP32 gestart\n");
}

void loop(void) {
    float lat = 0.0, lon = 0.0;
    if (parse_nmea((char*)test_sentence, &lat, &lon)) {
        printf("Lat: %.6f, Lon: %.6f\n", lat, lon);
    } else {
        printf("Geen geldige fix\n");
    }

    vTaskDelay(pdMS_TO_TICKS(1000)); // 1 seconde wachten
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
