#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <ttn.h>
#include <ttn_lmic.h>

// Pins voor FireBeetle ESP32 LoRa Cover (SX1276)
#define LORA_SCK     5
#define LORA_MISO    19
#define LORA_MOSI    27
#define LORA_NSS     18
#define LORA_RST     14
#define LORA_DIO0    26

// TTN configuratie - ingevuld voor OTAA
#define TTN_APP_EUI "0000000000000000"  // 8 bytes, hex
#define TTN_DEV_EUI "70B3D57ED0074065"  // 8 bytes, hex
#define TTN_APP_KEY "BB05FA024CFC992CE1E192BB7E261274"  // 16 bytes, hex

// Frequentieplan (EU of US)
#define TTN_FREQ_PLAN TTN_FP_EU868

static const char *TAG = "TTN_EXAMPLE";

// Globale variabelen
static ttn_t ttn_ctx;
static uint32_t counter = 0;

void setup_radio() {
    lmic_pinmap lmic_pins = {
        .nss = LORA_NSS,
        .rxtx = LMIC_UNUSED_PIN,
        .rst = LORA_RST,
        .dio = {LMIC_UNUSED_PIN, LORA_DIO0, LMIC_UNUSED_PIN, LMIC_UNUSED_PIN, LMIC_UNUSED_PIN}
    };

    ttn_lmic_radio_init(&lmic_pins, NULL);  // NULL voor SX1276 defaults
}

void app_send_callback(uint8_t *data, size_t len) {
    ESP_LOGI(TAG, "Data verstuurd: %u bytes", len);
}

void onReceive(uint8_t *data, size_t len, ttn_port_t port) {
    ESP_LOGI(TAG, "Bericht ontvangen op poort %d: %u bytes", port, len);
    // Verwerk downlink data hier indien nodig
}

void task_send(void *pvParameters) {
    char buffer[64];
    while (1) {
        counter++;
        snprintf(buffer, sizeof(buffer), "Hallo TTN! Counter: %lu", counter);

        ESP_LOGI(TAG, "Verzenden: %s", buffer);

        // Zend data (max 51 bytes voor LoRaWAN)
        ttn_send((uint8_t *)buffer, strlen(buffer), 1, true);  // Poort 1, confirm false

        vTaskDelay(pdMS_TO_TICKS(60000));  // Wacht 60 seconden (duty cycle respecteren!)
    }
}

void app_main() {
    // NVS initialiseren
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Starting TTN example");

    // Radio setup
    setup_radio();

    // TTN initialiseren (OTAA)
    ttn_init_otaa(&ttn_ctx, TTN_DEV_EUI, TTN_APP_EUI, TTN_APP_KEY, TTN_FREQ_PLAN);
    ttn_set_receive_callback(&ttn_ctx, onReceive);
    ttn_set_send_callback(&ttn_ctx, app_send_callback);

    // Start LoRaWAN stack
    ttn_start(&ttn_ctx);

    // Zend task starten
    xTaskCreate(task_send, "task_send", 4096, NULL, 5, NULL);
}