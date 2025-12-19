#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ttn.h"
#include "esp_sleep.h" // NODIG VOOR DEEP SLEEP

// --- ADC INCLUDES ---
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "NOX_DEEPSLEEP";

// ============================= CONFIG =============================
#define UART_NOX_NUM        UART_NUM_1
#define UART_NOX_RX_PIN     14
#define UART_NOX_TX_PIN     12
#define UART_NOX_BAUD       9600

#define GPS_UART_NUM        UART_NUM_2
#define GPS_RX_PIN          GPIO_NUM_17
#define GPS_TX_PIN          GPIO_NUM_16
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

#define ADC_UNIT            ADC_UNIT_1
#define ADC_CHANNEL         ADC_CHANNEL_0
#define ADC_ATTEN           ADC_ATTEN_DB_12
#define R1_TOP              100000.0f
#define R2_BOTTOM           270000.0f

// ============================= TTN KEYS =============================
const char *DEV_EUI = "70B3D57ED0074070";
const char *APP_EUI = "70B3D57ED0073702";
const char *APP_KEY = "B43689D3C3F09A6EA5C4A77A24B7D46D";

// ============================= INSTELLINGEN =============================
// Slaaptijd in minuten
#define SLEEP_MINUTES 15 

// Global vars (worden gereset na elke sleep!)
uint16_t nox_value = 0;
uint16_t current_battery_mv = 0;

typedef struct {
    double lat, lon;
    uint8_t hour, minute, second, day, month;
    uint16_t year;
    bool valid;
} gps_t;

gps_t gps = {0};

// ============================= HELPERS =============================

// Batterij meten (Single Shot)
void measure_battery() {
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = { .unit_id = ADC_UNIT };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));

    // Simpele kalibratie (zonder curve fitting om code klein te houden voor sleep)
    int adc_raw;
    if (adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_raw) == ESP_OK) {
        // Ruwe berekening: (Raw / 4095) * 3300mV * VoltageDivider
        float pin_mv = (adc_raw * 3300.0f) / 4095.0f;
        float batt_mv = pin_mv * ((R1_TOP + R2_BOTTOM) / R2_BOTTOM);
        current_battery_mv = (uint16_t)batt_mv;
    }
    // Opruimen
    adc_oneshot_del_unit(adc_handle);
}

// GPS Parsen
void parse_nmea(char *sentence) {
    // (Zelfde logica als voorheen, ingekort voor overzicht)
    char *tokens[20];
    int n = 0;
    char *p = strtok(sentence, ",");
    while (p && n < 20) { tokens[n++] = p; p = strtok(NULL, ","); }

    if (n >= 10 && strstr(sentence, "GGA")) {
        if (strlen(tokens[2]) == 0) return;
        double raw_lat = atof(tokens[2]);
        gps.lat = (int)(raw_lat/100) + (raw_lat - (int)(raw_lat/100)*100)/60.0;
        if (tokens[3][0] == 'S') gps.lat = -gps.lat;
        
        double raw_lon = atof(tokens[4]);
        gps.lon = (int)(raw_lon/100) + (raw_lon - (int)(raw_lon/100)*100)/60.0;
        if (tokens[5][0] == 'W') gps.lon = -gps.lon;
        
        gps.valid = true;
    }
}

// GPS Lezen (Met Timeout, want we willen niet eeuwig wachten)
void read_gps() {
    uart_config_t cfg = {
        .baud_rate = GPS_BAUD, .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE, .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(GPS_UART_NUM, &cfg);
    uart_set_pin(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, -1, -1);
    uart_driver_install(GPS_UART_NUM, GPS_BUF_SIZE*2, 0, 0, NULL, 0);

    ESP_LOGI(TAG, "GPS zoeken (max 5 sec)...");
    
    uint8_t *buf = malloc(GPS_BUF_SIZE);
    char line[256];
    int pos = 0;
    
    // We proberen 5 seconden lang GPS te krijgen. 
    // LET OP: Na Deep Sleep heeft GPS vaak "Cold Start" nodig (45s+).
    // Als je batterij wilt sparen, accepteer je vaak "geen GPS" tenzij je buiten bent.
    TickType_t start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < (5000 / portTICK_PERIOD_MS)) {
        int len = uart_read_bytes(GPS_UART_NUM, buf, GPS_BUF_SIZE-1, 100/portTICK_PERIOD_MS);
        if (len > 0) {
            for (int i=0; i<len; i++) {
                if (buf[i] == '\n') {
                    line[pos] = 0;
                    parse_nmea(line);
                    pos = 0;
                    if (gps.valid) break; // We hebben een fix!
                } else if (pos < 255) line[pos++] = buf[i];
            }
        }
        if (gps.valid) break;
    }
    free(buf);
    uart_driver_delete(GPS_UART_NUM);
}

// NOx Lezen
void read_nox() {
    uart_config_t uart_cfg = {
        .baud_rate = UART_NOX_BAUD, .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE, .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NOX_NUM, &uart_cfg);
    uart_set_pin(UART_NOX_NUM, UART_NOX_TX_PIN, UART_NOX_RX_PIN, -1, -1);
    uart_driver_install(UART_NOX_NUM, 256, 0, 0, NULL, 0);

    ESP_LOGI(TAG, "NOx sensor uitlezen...");
    uint8_t cmd[] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};
    uint8_t data[9];

    // Sensor even tijd geven om wakker te worden indien nodig
    vTaskDelay(pdMS_TO_TICKS(500));
    
    uart_flush(UART_NOX_NUM);
    uart_write_bytes(UART_NOX_NUM, cmd, 9);
    vTaskDelay(pdMS_TO_TICKS(200)); // Wacht op antwoord

    int len = uart_read_bytes(UART_NOX_NUM, data, 9, 200/portTICK_PERIOD_MS);
    if (len == 9 && data[0] == 0xFF && data[1] == 0x86) {
        nox_value = (data[2] << 8) | data[3];
        ESP_LOGI(TAG, "NOx Gemeten: %d", nox_value);
    } else {
        ESP_LOGW(TAG, "Geen NOx antwoord");
    }
    uart_driver_delete(UART_NOX_NUM);
}

// ============================= MAIN (One Pass Logic) =============================
void app_main(void)
{
    // --- STAP 1: VEILIGHEIDS PAUZE (ANTI-BRICK) ---
    // Dit is het allerbelangrijkste. Als je de chip reset, wacht hij hier 10 seconden.
    // In die tijd kan je nieuwe code uploaden als er iets mis is.
    ESP_LOGW(TAG, "STARTUP: Wacht 10 seconden (VEILIGHEID)... Druk nu flash als je wilt updaten.");
    vTaskDelay(pdMS_TO_TICKS(10000));
    ESP_LOGI(TAG, "Start Applicatie...");

    nvs_flash_init();
    
    // --- STAP 2: SENSOREN LEZEN ---
    measure_battery();
    read_gps();
    read_nox();

    // --- STAP 3: LORA INIT & VERZENDEN ---
    spi_bus_config_t bus = {
        .mosi_io_num = LORA_PIN_MOSI, .miso_io_num = LORA_PIN_MISO,
        .sclk_io_num = LORA_PIN_SCK, .quadwp_io_num = -1, .quadhd_io_num = -1
    };
    spi_bus_initialize(LORA_SPI_HOST, &bus, SPI_DMA_CH_AUTO);
    gpio_install_isr_service(0);

    ttn_init();
    ttn_configure_pins(LORA_SPI_HOST, LORA_PIN_NSS, TTN_NOT_CONNECTED, LORA_PIN_RST, LORA_PIN_DIO0, LORA_PIN_DIO1);
    ttn_provision(DEV_EUI, APP_EUI, APP_KEY);
    
    // Probeer te joinen
    ESP_LOGI(TAG, "Joining TTN...");
    if (ttn_join()) {
        ESP_LOGI(TAG, "Joined! Verzenden...");
        
        uint8_t payload[20];
        int len = 0;

        // NOx
        payload[len++] = nox_value >> 8;
        payload[len++] = nox_value & 0xFF;

        // GPS
        if (gps.valid) {
            payload[len++] = 1;
            int32_t lat = (int32_t)(gps.lat * 1000000);
            payload[len++] = (lat >> 24) & 0xFF; payload[len++] = (lat >> 16) & 0xFF;
            payload[len++] = (lat >> 8) & 0xFF; payload[len++] = lat & 0xFF;
            int32_t lon = (int32_t)(gps.lon * 1000000);
            payload[len++] = (lon >> 24) & 0xFF; payload[len++] = (lon >> 16) & 0xFF;
            payload[len++] = (lon >> 8) & 0xFF; payload[len++] = lon & 0xFF;
        } else {
            payload[len++] = 0; 
        }

        // Batt
        payload[len++] = (current_battery_mv >> 8) & 0xFF;
        payload[len++] = current_battery_mv & 0xFF;

        ttn_transmit_message(payload, len, 1, false);
        
        // Wacht even tot de transmissie echt weg is (belangrijk voor sleep!)
        vTaskDelay(pdMS_TO_TICKS(5000)); 
    } else {
        ESP_LOGE(TAG, "Join mislukt (geen bereik?)");
    }

    // --- STAP 4: DEEP SLEEP ---
    ESP_LOGI(TAG, "Klaar. Slaap nu voor %d minuten. Tot straks!", SLEEP_MINUTES);
    
    // Zet de timer
    esp_sleep_enable_timer_wakeup(SLEEP_MINUTES * 60 * 1000000ULL);
    
    // Start Deep Sleep (De chip gaat nu UIT)
    esp_deep_sleep_start();
}