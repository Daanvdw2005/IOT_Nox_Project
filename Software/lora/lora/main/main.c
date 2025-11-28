#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ttn.h"
#include <math.h> // Voor abs()

// ============================= CONFIG =============================
#define UART_CO2_NUM        UART_NUM_1
#define UART_CO2_RX_PIN     16
#define UART_CO2_TX_PIN     17
#define UART_CO2_BAUD       9600

#define GPS_UART_NUM        UART_NUM_2
#define GPS_RX_PIN          GPIO_NUM_14       // Correct, weg van GPIO 25
#define GPS_TX_PIN          GPIO_NUM_12     // Opgelost: Weg van GPIO 26 (DIO0)
#define GPS_BAUD            9600
#define GPS_BUF_SIZE        1024

#define LORA_SPI_HOST       SPI2_HOST
#define LORA_PIN_MISO       GPIO_NUM_19
#define LORA_PIN_MOSI       GPIO_NUM_23
#define LORA_PIN_SCK        GPIO_NUM_18
#define LORA_PIN_NSS        GPIO_NUM_27
#define LORA_PIN_RST        GPIO_NUM_25
#define LORA_PIN_DIO0       GPIO_NUM_26
#define LORA_PIN_DIO1       GPIO_NUM_9

static const char *TAG = "NOX";

// === JOUW TTN KEYS (OTAA) ===
const char *DEV_EUI = "70B3D57ED0074070";
const char *APP_EUI = "70B3D57ED0073702";
const char *APP_KEY = "B43689D3C3F09A6EA5C4A77A24B7D46D";

// === UPLINK CONTROLE ELKE 30 SECONDE (Delta/Heartbeat) ===
#define UPLINK_INTERVAL_MS 30000UL

// ============================= DATA =============================
volatile uint16_t co2_ppm = 0;
volatile bool co2_ready = false;

// Variabelen voor State Change verzending (Delta)
volatile uint16_t last_sent_co2 = 0; 
#define CO2_CHANGE_THRESHOLD 20 // OPGESCHOOND VAN VREEMDE TEKENS
// GEEN VREEMDE TEKENS MEER OP LIJN 52!

typedef struct {
    float lat, lon;
    uint8_t hour, minute, second, day, month;
    uint16_t year;
    bool valid;
} gps_t;

gps_t gps = {0};

// ============================= NMEA PARSER =============================
static void parse_nmea(char *sentence)
{
    char *tokens[20];
    int n = 0;
    char *p = strtok(sentence, ",");
    while (p && n < 20) { tokens[n++] = p; p = strtok(NULL, ","); }

    if (n >= 10 && strstr(sentence, "GGA")) {
        if (strlen(tokens[2]) < 4 || strlen(tokens[4]) < 4) return;

        int lat_deg = atoi(tokens[2]) / 100;
        float lat_min = atof(tokens[2] + 2) / 100.0f;
        gps.lat = lat_deg + lat_min / 60.0f;
        if (tokens[3][0] == 'S') gps.lat = -gps.lat;

        int lon_deg = atoi(tokens[4]) / 100;
        float lon_min = atof(tokens[4] + (strlen(tokens[4]) > 7 ? 3 : 2)) / 100.0f;
        gps.lon = lon_deg + lon_min / 60.0f;
        if (tokens[5][0] == 'W') gps.lon = -gps.lon;

        if (strlen(tokens[1]) >= 6) {
            gps.hour   = (tokens[1][0]-'0')*10 + (tokens[1][1]-'0');
            gps.minute = (tokens[1][2]-'0')*10 + (tokens[1][3]-'0');
            gps.second = (tokens[1][4]-'0')*10 + (tokens[1][5]-'0');
        }
        gps.valid = true;
    }

    if (n >= 10 && strstr(sentence, "RMC") && strlen(tokens[9]) == 6) {
        gps.day   = (tokens[9][0]-'0')*10 + (tokens[9][1]-'0');
        gps.month = (tokens[9][2]-'0')*10 + (tokens[9][3]-'0');
        gps.year  = 2000 + (tokens[9][4]-'0')*10 + (tokens[9][5]-'0');
    }
}

// ============================= GPS TASK =============================
void gps_task(void *pvParameters)
{
    uint8_t *buf = malloc(GPS_BUF_SIZE);
    char line[256];
    int pos = 0;

    uart_config_t cfg = {
        .baud_rate = GPS_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(GPS_UART_NUM, &cfg);
    uart_set_pin(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, -1, -1);
    uart_driver_install(GPS_UART_NUM, GPS_BUF_SIZE*2, 0, 0, NULL, 0);

    ESP_LOGI(TAG, "GPS gestart op UART2 (RX=GPIO%d)", GPS_RX_PIN);

    TickType_t last_log = 0;

    while (1) {
        int len = uart_read_bytes(GPS_UART_NUM, buf, GPS_BUF_SIZE-1, 50/portTICK_PERIOD_MS);
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                char c = buf[i];
                if (c == '\n' || c == '\r') {
                    if (pos > 10 && line[0] == '$') {
                        line[pos] = '\0';
                        parse_nmea(line);
                    }
                    pos = 0;
                } else if (pos < (int)sizeof(line)-1) {
                    line[pos++] = c;
                }
            }
        }

        if (xTaskGetTickCount() - last_log >= 1000/portTICK_PERIOD_MS) {
            if (gps.valid) {
                ESP_LOGI(TAG, "GPS: %.6f, %.6f | %02d:%02d:%02d %02d-%02d-%d",
                         gps.lat, gps.lon, gps.hour, gps.minute, gps.second,
                         gps.day, gps.month, gps.year);
            } else {
                ESP_LOGI(TAG, "GPS: geen fix");
            }
            last_log = xTaskGetTickCount();
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// ============================= CO2 TASK (Gecorrigeerd Commando & Logica) =============================
void co2_task(void *arg)
{
    // Juiste commando voor het LEZEN van de CO2-concentratie (0x86 functie voor TB600B/MH-Z19)
    uint8_t read_cmd[] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79}; 
    uint8_t data[9];
    
    while (1) {
        // 1. Zorg dat de buffer leeg is om oude data te vermijden
        uart_flush(UART_CO2_NUM);
        
        // 2. Stuur het leescommando naar de TB600B
        uart_write_bytes(UART_CO2_NUM, read_cmd, 9);
        
        // 3. Wacht iets langer op het antwoord voor betrouwbaarheid
        vTaskDelay(pdMS_TO_TICKS(250)); 

        // 4. Probeer 9 bytes te lezen met een langere timeout
        int len = uart_read_bytes(UART_CO2_NUM, data, 9, 250/portTICK_PERIOD_MS);
        
        if (len == 9) { 
            if (data[0] == 0xFF && data[1] == 0x86) {
                uint8_t sum = 0;
                for (int i = 0; i < 8; i++) sum += data[i];
                
                if ((0xFF - sum) == data[8]) {
                    co2_ppm = (data[2] << 8) | data[3];
                    co2_ready = true;
                    ESP_LOGI(TAG, "CO2: %d ppm", co2_ppm);
                } else {
                    ESP_LOGE(TAG, "CO2: Checksum Fout. Data genegeerd.");
                }
            } else {
                ESP_LOGE(TAG, "CO2: Onverwacht Antwoord Type. Ontvangen type: %02X %02X", data[0], data[1]);
            }
        } else {
            ESP_LOGE(TAG, "CO2: Geen of onvolledig antwoord (%d bytes ontvangen)", len);
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Lees elke seconde
    }
}

// ============================= TTN UPLINK TASK (met Delta Check en Heartbeat) =============================
void ttn_task(void *arg)
{
    while (!co2_ready) {
        ESP_LOGI(TAG, "Wachten op eerste CO2-meting...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    ESP_LOGI(TAG, "CO2 klaar (%d ppm) -> controle elke 30 sec (versturen bij %d ppm verandering)", co2_ppm, CO2_CHANGE_THRESHOLD);

    // Initialiseer last_sent_co2 om de eerste zending te garanderen
    last_sent_co2 = co2_ppm - CO2_CHANGE_THRESHOLD - 1; 

    uint32_t counter = 0;
    uint32_t heartbeat_counter = 0;
    const uint32_t HEARTBEAT_LIMIT = 120; // 120 cycli * 30 sec = 1 uur heartbeat

    while (1) {
        bool should_send = false;
        
        // 1. Controleer Delta (ppm verandering)
        if (abs(co2_ppm - last_sent_co2) >= CO2_CHANGE_THRESHOLD) {
            should_send = true;
            ESP_LOGI(TAG, "TRIG: CO2-verandering > %d ppm", CO2_CHANGE_THRESHOLD);
        }

        // 2. Controleer Heartbeat (eenmaal per uur)
        if (++heartbeat_counter >= HEARTBEAT_LIMIT) {
            should_send = true;
            heartbeat_counter = 0;
            ESP_LOGI(TAG, "TRIG: Heartbeat (1 uur)");
        }

        // 3. Verstuur indien nodig
        if (should_send) {
            uint8_t payload[11];
            int len = 2;

            payload[0] = co2_ppm >> 8;
            payload[1] = co2_ppm & 0xFF;

            if (gps.valid) {
                payload[len++] = 1;
                int32_t lat = (int32_t)(gps.lat * 1000000);
                int32_t lon = (int32_t)(gps.lon * 1000000);
                payload[len++] = (lat >> 24) & 0xFF;
                payload[len++] = (lat >> 16) & 0xFF;
                payload[len++] = (lat >> 8) & 0xFF;
                payload[len++] = lat & 0xFF;
                payload[len++] = (lon >> 24) & 0xFF;
                payload[len++] = (lon >> 16) & 0xFF;
                payload[len++] = (lon >> 8) & 0xFF;
                payload[len++] = lon & 0xFF;
            } else {
                payload[len++] = 0;
            }

            ESP_LOGI(TAG, "Uplink #%lu | CO2: %d ppm | GPS: %s", ++counter, co2_ppm, gps.valid?"JA":"NEE");
            ttn_transmit_message(payload, len, 1, false); 
            
            // Update de laatst verzonden waarde NA succesvolle verzending
            last_sent_co2 = co2_ppm; 
        } else {
            ESP_LOGI(TAG, "Geen significante verandering. Wachten op volgende cyclus.");
        }

        vTaskDelay(pdMS_TO_TICKS(UPLINK_INTERVAL_MS)); // Wacht 30 seconden
    }
}

// ============================= MAIN =============================
void app_main(void)
{
    ESP_LOGI(TAG, "=== NOX LoRa Sensor Start ===");

    nvs_flash_init();

    spi_bus_config_t bus = {
        .mosi_io_num = LORA_PIN_MOSI,
        .miso_io_num = LORA_PIN_MISO,
        .sclk_io_num = LORA_PIN_SCK,
    };
    spi_bus_initialize(LORA_SPI_HOST, &bus, SPI_DMA_CH_AUTO);
    gpio_install_isr_service(0);

    ttn_init();
    ttn_configure_pins(LORA_SPI_HOST, LORA_PIN_NSS, TTN_NOT_CONNECTED,
                       LORA_PIN_RST, LORA_PIN_DIO0, LORA_PIN_DIO1);
    ttn_provision(DEV_EUI, APP_EUI, APP_KEY);
    ttn_set_data_rate(TTN_DR_EU868_SF7_BW125);

    if (!ttn_join()) {
        ESP_LOGE(TAG, "TTN Join mislukt!");
        return;
    }
    ESP_LOGI(TAG, "Verbonden met TTN");

    // CO2 UART
    uart_config_t uart_cfg = {
        .baud_rate = UART_CO2_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_CO2_NUM, &uart_cfg);
    uart_set_pin(UART_CO2_NUM, UART_CO2_TX_PIN, UART_CO2_RX_PIN, -1, -1);
    uart_driver_install(UART_CO2_NUM, 256, 0, 0, NULL, 0);

    // Start taken
    xTaskCreate(co2_task, "co2", 4096, NULL, 10, NULL);
    xTaskCreate(gps_task, "gps", 4096, NULL, 8, NULL);
    xTaskCreate(ttn_task, "ttn", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Alles draait - eerste uplink over ~30 seconden (indien CO2-verandering)");
}