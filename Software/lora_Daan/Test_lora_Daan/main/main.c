#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lmic.h"
#include "hal.h"
#include <string.h>

static const char *TAG = "TTN_ABP";

// -----------------------------------------------------------------------------
// --- TTN ABP KEYS (MSB) ---
// -----------------------------------------------------------------------------

// Vul hier je eigen sleutels van TTN in!
// (Let op: hexadecimale bytes, gescheiden door komma’s)

static u1_t NWKSKEY[16] = {
    0x10, 0x25, 0xB6, 0x3D, 0x4A, 0x4C, 0x0F, 0xA6,
    0x64, 0x18, 0xDC, 0x1A, 0xC1, 0xAA, 0x00, 0x00
};

static u1_t APPSKEY[16] = {
    0x7F, 0xB5, 0x95, 0xA5, 0x1F, 0xAB, 0x29, 0x61,
    0x73, 0x7C, 0x3B, 0x51, 0x42, 0xCC, 0xA3, 0x5E
};

// Device Address (MSB)
static const u4_t DEVADDR = 0x260B0AFE;

// -----------------------------------------------------------------------------
// --- PIN MAPPING: FireBeetle ESP32 + LoRa Cover (SX1276 868 MHz) ---
// -----------------------------------------------------------------------------
const lmic_pinmap lmic_pins = {
    .nss = 4,                     // SPI chip select
    .rxtx = LMIC_UNUSED_PIN,      // Not used
    .rst = 2,                     // Reset pin
    .dio = {5, LMIC_UNUSED_PIN, LMIC_UNUSED_PIN}, // DIO0 only
};

// -----------------------------------------------------------------------------
// --- Bericht versturen ---
// -----------------------------------------------------------------------------
static void do_send(osjob_t* j) {
    static uint8_t message[] = "Hello from FireBeetle!";
    if (LMIC.opmode & OP_TXRXPEND) {
        ESP_LOGW(TAG, "TX pending, not sending");
    } else {
        LMIC_setTxData2(1, message, sizeof(message) - 1, 0);
        ESP_LOGI(TAG, "Packet queued");
    }
    // De timer wordt in onEvent opnieuw ingesteld, dus hier niet nodig
}

// -----------------------------------------------------------------------------
// --- LoRaWAN Callback Functies ---
// -----------------------------------------------------------------------------
void onEvent(ev_t ev) {
    switch (ev) {
        case EV_TXCOMPLETE:
            ESP_LOGI(TAG, "Transmission complete");
            if (LMIC.txrxFlags & TXRX_ACK)
                ESP_LOGI(TAG, "Received ACK");
            if (LMIC.dataLen) {
                ESP_LOGI(TAG, "Received %d bytes of payload", LMIC.dataLen);
            }
            // Plan de volgende verzending na 30 seconden
            os_setTimedCallback(&LMIC.osjob, os_getTime() + sec2osticks(30), do_send);
            break;
        default:
            ESP_LOGI(TAG, "Event: %d", ev);
            break;
    }
}

// -----------------------------------------------------------------------------
// --- main() ---
// -----------------------------------------------------------------------------
void app_main(void) {
    ESP_LOGI(TAG, "Starting LMIC LoRaWAN (ABP mode)");

    // LMIC initialiseren
    os_init();
    LMIC_reset();

    // Stel de ABP sessie in
    LMIC_setSession(0x1, DEVADDR, NWKSKEY, APPSKEY);

    // Configureer EU868 kanalen
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI); // 868.1 MHz
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI); // 868.3 MHz
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI); // 868.5 MHz
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI); // 867.1 MHz
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI); // 867.3 MHz
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI); // 867.5 MHz
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI); // 867.7 MHz
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI); // 867.9 MHz

    // Uitschakelen van link-checks
    LMIC_setLinkCheckMode(0);

    // Stel datarate en zendvermogen in
    LMIC_setDrTxpow(DR_SF7, 14);

    // Eerste bericht verzenden
    do_send(&LMIC.osjob);

    while (1) {
        os_runloop_once();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}