// start-------------------------------------------test-GPS-------------------------------------------------
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

// UART configuratie
#define UART_NUM UART_NUM_1
#define BUF_SIZE (1024)
// versie 2
// #define TXD_PIN (GPIO_NUM_26)
// #define RXD_PIN (GPIO_NUM_25)

// versie 1
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)

// Globale variabelen voor GPS-data
typedef struct {
    float latitude;
    float longitude;
    float altitude;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} gps_data_t;

gps_data_t gps_data;

void parse_nmea(char *nmea_sentence) {
    // Hier kun je de NMEA-zinnen parsen om latitude, longitude, tijd, etc. te extraheren.
    // Voor een volledige implementatie kun je de TinyGPS++-logica overnemen.
    // Voorbeeld: $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
    // Deze functie is een placeholder; je moet hem uitbreiden voor echte NMEA-parsing.
}

void init_uart() {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void gps_task(void *pvParameters) {
    uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
    while (1) {
        int length = uart_read_bytes(UART_NUM, data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (length > 0) {
            data[length] = '\0';
            printf("Received: %s\n", data);
            parse_nmea((char *)data);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    free(data);
}

void app_main() {
    init_uart();
    xTaskCreate(gps_task, "gps_task", 4096, NULL, 10, NULL);
}
//end-------------------------------------------test-GPS-------------------------------------------------

//start-----------------------------------------test-TB600B-------------------------------------------------
// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/uart.h"
// #include "driver/gpio.h"
// #include "esp_err.h"

// #define UART_PORT_NUM      UART_NUM_1
// #define UART_BAUD_RATE     9600
// #define UART_RX_PIN        16
// #define UART_TX_PIN        17
// #define UART_BUF_SIZE      128
// #define TASK_STACK_SIZE    4096
// #define TASK_PRIORITY      10

// // Zet sensor in automatische rapportagemodus
// void switch_to_auto_reporting_mode() {
//     uint8_t cmd[] = {0xFF, 0x01, 0x78, 0x40, 0x00, 0x00, 0x00, 0x00, 0x47};
//     uart_write_bytes(UART_PORT_NUM, cmd, sizeof(cmd));
//     vTaskDelay(1000 / portTICK_PERIOD_MS);
// }

// // Parse TB600B data
// void parse_tb600b_data(uint8_t *data, size_t len) {
//     if (len < 9) return;

//     printf("Ontvangen bytes: ");
//     for (int i = 0; i < len; i++) {
//         printf("%02X ", data[i]);
//     }
//     printf("\n");

//     if (data[0] == 0xFF && data[1] == 0x86) {
//         uint16_t conc2 = (data[2] << 8) | data[3];
//         uint16_t range = (data[4] << 8) | data[5];
//         uint16_t conc1 = (data[6] << 8) | data[7];
//         uint8_t checksum = data[8];

//         // ✅ Correcte checksum-berekening (zonder +1)
//         uint16_t sum = 0;
//         for (int i = 0; i < 8; i++) {
//             sum += data[i];
//         }
//         uint8_t calculated_checksum = (0xFF - (sum & 0xFF)) & 0xFF;

//         if (calculated_checksum == checksum) {
//             printf("Concentratie-2: %d ppm\n", conc2);
//             printf("Range: %d\n", range);
//             printf("Concentratie-1: %d ppm\n", conc1);
//         } else {
//             printf("Checksum-fout! Ontvangen: %02X, Berekend: %02X\n", checksum, calculated_checksum);
//         }
//     } else {
//         printf("Onbekend pakket!\n");
//     }
// }

// // Task die seriële data uitleest
// static void uart_read_task(void *arg) {
//     uint8_t data[UART_BUF_SIZE];
//     int pos = 0;

//     printf("UART read task gestart...\n");

//     switch_to_auto_reporting_mode();

//     while (1) {
//         int len = uart_read_bytes(UART_PORT_NUM, &data[pos], 1, 20 / portTICK_PERIOD_MS);
//         if (len > 0) {
//             pos += len;

//             // Elke meting bestaat uit 9 bytes
//             if (pos >= 9) {
//                 parse_tb600b_data(data, pos);
//                 pos = 0;  // reset buffer voor volgende pakket
//             }
//         }
//         vTaskDelay(10 / portTICK_PERIOD_MS);
//     }
// }

// // Hoofdfunctie
// void app_main(void) {
//     uart_config_t uart_config = {
//         .baud_rate = UART_BAUD_RATE,
//         .data_bits = UART_DATA_8_BITS,
//         .parity = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
//     };

//     ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
//     ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
//     ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0));

//     printf("ESP32 TB600B lezer gestart. Wacht op data...\n");

//     xTaskCreate(uart_read_task, "uart_read_task", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
// }
//end-----------------------------------------test-TB600B-------------------------------------------------

// //start-----------------------------------------werkend-zonder-tijd-------------------------------------------------

// #include <stdio.h>
// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/uart.h"
// #include "driver/gpio.h"
// #include "esp_err.h"

// // TB600B
// #define UART_TB600B_NUM   UART_NUM_1
// #define TB600B_TX_PIN     GPIO_NUM_17
// #define TB600B_RX_PIN     GPIO_NUM_16

// // // GPS-module V1
// // #define UART_GPS_NUM      UART_NUM_2
// // #define GPS_TX_PIN        GPIO_NUM_26
// // #define GPS_RX_PIN        GPIO_NUM_27

// // GPS-module V2
// #define UART_GPS_NUM      UART_NUM_2
// #define GPS_TX_PIN        GPIO_NUM_26
// #define GPS_RX_PIN        GPIO_NUM_25

// #define UART_BAUD_RATE    9600
// #define UART_BUF_SIZE     256
// #define TASK_STACK_SIZE   4096
// #define TASK_PRIORITY     10     

// // -----------------------------
// // TB600B functies
// // -----------------------------

// void switch_to_auto_reporting_mode() {
//     uint8_t cmd[] = {0xFF, 0x01, 0x78, 0x40, 0x00, 0x00, 0x00, 0x00, 0x47};
//     uart_write_bytes(UART_TB600B_NUM, cmd, sizeof(cmd));
//     vTaskDelay(1000 / portTICK_PERIOD_MS);
// }

// void parse_tb600b_data(uint8_t *data, size_t len) {
//     if (len < 9) return;

//     if (data[0] == 0xFF && data[1] == 0x86) {
//         uint16_t conc2 = (data[2] << 8) | data[3];
//         uint16_t range = (data[4] << 8) | data[5];
//         uint16_t conc1 = (data[6] << 8) | data[7];
//         uint8_t checksum = data[8];

//         uint16_t sum = 0;
//         for (int i = 0; i < 8; i++) sum += data[i];
//         uint8_t calc_checksum = (0xFF - (sum & 0xFF)) & 0xFF;

//         if (calc_checksum == checksum) {
//             printf("[TB600B] CO₂: %d ppm, Range: %d, Backup: %d ppm\n",
//                    conc2, range, conc1);
//         } else {
//             printf("[TB600B] Checksum-fout! Ontvangen: %02X, Berekend: %02X\n",
//                    checksum, calc_checksum);
//         }
//     }
// }

// void tb600b_task(void *arg) {
//     uint8_t data[UART_BUF_SIZE];
//     int pos = 0;
//     printf("[TB600B] UART task gestart...\n");
//     switch_to_auto_reporting_mode();

//     while (1) {
//         int len = uart_read_bytes(UART_TB600B_NUM, &data[pos], 1, 20 / portTICK_PERIOD_MS);
//         if (len > 0) {
//             pos += len;
//             if (pos >= 9) {
//                 parse_tb600b_data(data, pos);
//                 pos = 0;
//             }
//         }
//         vTaskDelay(10 / portTICK_PERIOD_MS);
//     }
// }

// // -----------------------------
// // GPS functies
// // -----------------------------

// void parse_nmea(char *nmea_sentence) {
//     if (strncmp(nmea_sentence, "$GPGGA", 6) == 0) {
//         printf("[GPS] GPGGA-zin ontvangen: %s\n", nmea_sentence);
//     } else if (strncmp(nmea_sentence, "$GPRMC", 6) == 0) {
//         printf("[GPS] GPRMC-zin ontvangen: %s\n", nmea_sentence);
//     }
// }

// void gps_task(void *arg) {
//     uint8_t data[UART_BUF_SIZE];
//     int len;
//     printf("[GPS] UART task gestart...\n");

//     while (1) {
//         len = uart_read_bytes(UART_GPS_NUM, data, sizeof(data) - 1, 100 / portTICK_PERIOD_MS);
//         if (len > 0) {
//             data[len] = '\0';
//             char *token = strtok((char *)data, "\r\n");
//             while (token != NULL) {
//                 parse_nmea(token);
//                 token = strtok(NULL, "\r\n");
//             }
//         }
//         vTaskDelay(100 / portTICK_PERIOD_MS);
//     }
// }

// // -----------------------------
// // UART-initialisatie
// // -----------------------------

// void init_uart(uart_port_t uart_num, int tx, int rx) {
//     uart_config_t uart_config = {
//         .baud_rate = UART_BAUD_RATE,
//         .data_bits = UART_DATA_8_BITS,
//         .parity = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
//         .source_clk = UART_SCLK_APB,
//     };

//     ESP_ERROR_CHECK(uart_driver_install(uart_num, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
//     ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
//     ESP_ERROR_CHECK(uart_set_pin(uart_num, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
// }

// // -----------------------------
// // Hoofdfunctie
// // -----------------------------

// void app_main(void) {
//     printf("ESP32 FireBeetle gestart — TB600B + GPS via UART\n");

//     // Init beide UART's
//     init_uart(UART_TB600B_NUM, TB600B_TX_PIN, TB600B_RX_PIN);
//     init_uart(UART_GPS_NUM, GPS_TX_PIN, GPS_RX_PIN);

//     // Start taken
//     xTaskCreate(tb600b_task, "tb600b_task", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
//     xTaskCreate(gps_task, "gps_task", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
// }
// //  -----------------------------------------werkend-zonder-tijd-------------------------------------------------

// #include <stdio.h>
// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/uart.h"
// #include "driver/gpio.h"
// #include "esp_err.h"

// // ===============================================
// // CONFIG
// // ===============================================

// // TB600B
// #define UART_TB600B_NUM   UART_NUM_1
// #define TB600B_TX_PIN     GPIO_NUM_17
// #define TB600B_RX_PIN     GPIO_NUM_16

// // GPS-module
// #define UART_GPS_NUM      UART_NUM_2
// #define GPS_TX_PIN        GPIO_NUM_26
// #define GPS_RX_PIN        GPIO_NUM_25

// #define UART_BAUD_RATE    9600
// #define UART_BUF_SIZE     256
// #define TASK_STACK_SIZE   4096
// #define TASK_PRIORITY     10

// // ===============================================
// // GLOBALE STRUCT MET DATA
// // ===============================================

// typedef struct {
//     float latitude;
//     float longitude;

//     uint8_t hour;
//     uint8_t minute;
//     uint8_t second;

//     uint8_t day;
//     uint8_t month;
//     uint16_t year;

//     uint16_t co2_ppm;
//     bool gps_valid;

// } measurement_t;

// measurement_t current_data;

// // ===============================================
// // TB600B SENSOR
// // ===============================================

// void switch_to_auto_reporting_mode() {
//     uint8_t cmd[] = {0xFF, 0x01, 0x78, 0x40, 0x00, 0x00, 0x00, 0x00, 0x47};
//     uart_write_bytes(UART_TB600B_NUM, cmd, sizeof(cmd));
//     vTaskDelay(1000 / portTICK_PERIOD_MS);
// }

// void print_measurement() {
//     printf("\n=========== METING ===========\n");
//     printf("CO₂: %d ppm\n", current_data.co2_ppm);

//     if (current_data.gps_valid) {
//         printf("GPS Latitude : %.6f\n", current_data.latitude);
//         printf("GPS Longitude: %.6f\n", current_data.longitude);

//         printf("Tijd : %02d:%02d:%02d\n",
//                current_data.hour,
//                current_data.minute,
//                current_data.second);

//         printf("Datum: %02d-%02d-%04d\n",
//                current_data.day,
//                current_data.month,
//                current_data.year);
//     } else {
//         printf("GPS nog niet geldig (no fix)\n");
//     }

//     printf("================================\n\n");
// }

// void parse_tb600b_data(uint8_t *data, size_t len) {
//     if (len < 9) return;

//     if (data[0] == 0xFF && data[1] == 0x86) {

//         uint16_t conc = (data[2] << 8) | data[3];
//         uint8_t checksum = data[8];

//         uint16_t sum = 0;
//         for (int i = 0; i < 8; i++) sum += data[i];
//         uint8_t calc_checksum = (0xFF - (sum & 0xFF)) & 0xFF;

//         if (calc_checksum == checksum) {

//             current_data.co2_ppm = conc;
//             print_measurement();

//         } else {
//             printf("[TB600B] Checksum fout!\n");
//         }
//     }
// }

// void tb600b_task(void *arg) {
//     uint8_t data[UART_BUF_SIZE];
//     int pos = 0;

//     printf("[TB600B] Task gestart...\n");

//     switch_to_auto_reporting_mode();

//     while (1) {
//         int len = uart_read_bytes(UART_TB600B_NUM, &data[pos], 1, 20 / portTICK_PERIOD_MS);
//         if (len > 0) {
//             pos += len;

//             if (pos >= 9) {
//                 parse_tb600b_data(data, pos);
//                 pos = 0;
//             }
//         }
//         vTaskDelay(10 / portTICK_PERIOD_MS);
//     }
// }

// // ===============================================
// // GPS PARSER (GPRMC)
// // ===============================================

// void parse_gprmc(char *nmea) {
//     char *tok = strtok(nmea, ",");
//     int field = 0;

//     while (tok != NULL) {
//         switch (field) {

//             case 1: { // tijd hhmmss
//                 int t = atoi(tok);
//                 current_data.hour = t / 10000;
//                 current_data.minute = (t / 100) % 100;
//                 current_data.second = t % 100;
//                 break;
//             }
//             case 2: { // A = valid, V = invalid
//                 current_data.gps_valid = (tok[0] == 'A');
//                 break;
//             }
//             case 3: { // latitude ddmm.mmmm
//                 float raw = atof(tok);
//                 int deg = raw / 100;
//                 float min = raw - (deg * 100);
//                 current_data.latitude = deg + (min / 60.0);
//                 break;
//             }
//             case 4: { // N/S
//                 if (tok[0] == 'S') current_data.latitude *= -1;
//                 break;
//             }
//             case 5: { // longitude dddmm.mmmm
//                 float raw = atof(tok);
//                 int deg = raw / 100;
//                 float min = raw - (deg * 100);
//                 current_data.longitude = deg + (min / 60.0);
//                 break;
//             }
//             case 6: { // E/W
//                 if (tok[0] == 'W') current_data.longitude *= -1;
//                 break;
//             }
//             case 9: { // datum ddmmyy
//                 int d = atoi(tok);
//                 current_data.day   = d / 10000;
//                 current_data.month = (d / 100) % 100;
//                 current_data.year  = 2000 + (d % 100);
//                 break;
//             }
//         }

//         tok = strtok(NULL, ",");
//         field++;
//     }
// }

// // ===============================================
// // GPS TASK
// // ===============================================

// void gps_task(void *arg) {
//     printf("[GPS] Task gestart...\n");

//     uint8_t data[UART_BUF_SIZE];

//     while (1) {
//         int len = uart_read_bytes(UART_GPS_NUM, data, sizeof(data)-1, 100 / portTICK_PERIOD_MS);
//         if (len > 0) {

//             data[len] = '\0';
//             char *line = strtok((char *)data, "\r\n");

//             while (line != NULL) {
//                 if (strncmp(line, "$GPRMC", 6) == 0) {
//                     parse_gprmc(line);
//                 }
//                 line = strtok(NULL, "\r\n");
//             }
//         }
//         vTaskDelay(20 / portTICK_PERIOD_MS);
//     }
// }

// // ===============================================
// // UART INIT
// // ===============================================

// void init_uart(uart_port_t uart_num, int tx, int rx) {
//     uart_config_t cfg = {
//         .baud_rate = UART_BAUD_RATE,
//         .data_bits = UART_DATA_8_BITS,
//         .parity = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
//         .source_clk = UART_SCLK_APB,
//     };

//     ESP_ERROR_CHECK(uart_driver_install(uart_num, UART_BUF_SIZE*2, 0, 0, NULL, 0));
//     ESP_ERROR_CHECK(uart_param_config(uart_num, &cfg));
//     ESP_ERROR_CHECK(uart_set_pin(uart_num, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
// }

// // ===============================================
// // MAIN
// // ===============================================

// void app_main(void) {

//     printf("\nESP32 gestart — TB600B + GPS NMEA parser + koppeling\n");

//     init_uart(UART_TB600B_NUM, TB600B_TX_PIN, TB600B_RX_PIN);
//     init_uart(UART_GPS_NUM, GPS_TX_PIN, GPS_RX_PIN);

//     xTaskCreate(tb600b_task, "tb600b_task", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
//     xTaskCreate(gps_task,    "gps_task",    TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
// }
