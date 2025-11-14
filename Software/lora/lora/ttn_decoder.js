#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ttn.h"

static const char *TAG = "lorawan";

// --- LoRa radio pin mapping for FireBeetle ESP32 + LoRa cover 868MHz v1.0 ----
#define LORA_SPI_HOST SPI2_HOST
#define LORA_PIN_MISO GPIO_NUM_19
#define LORA_PIN_MOSI GPIO_NUM_23
#define LORA_PIN_SCK GPIO_NUM_18
#define LORA_PIN_NSS GPIO_NUM_27   // CS / NSS
#define LORA_PIN_RST GPIO_NUM_25   // RST
#define LORA_PIN_DIO0 GPIO_NUM_26  // DIO0
#define LORA_PIN_DIO1 GPIO_NUM_9   // DIO1 (some boards leave this unconnected)

// --- TTN credentials (replace with values from the TTN console) --------------
static const char *const DEV_EUI = "70B3D57ED0074070";
static const char *const APP_EUI = "70B3D57ED0073702";  // TODO: Vul AppEUI/JoinEUI in vanuit TTN console (niet zichtbaar in afbeelding)
static const char *const APP_KEY = "B43689D3C3F09A6EA5C4A77A24B7D46D";  // Let op: dit is AppSKey uit sessie, niet AppKey! Controleer TTN console voor juiste AppKey

// Delay between uplinks
#define UPLINK_INTERVAL_MS (60 * 1000)

static void log_hex_payload(const char *prefix, const uint8_t *payload, size_t length)
{
    if (length == 0)
    {
        ESP_LOGI(TAG, "%s: <leegPayload>", prefix);
        return;
    }

    char hex[(length * 2) + 1];
    for (size_t i = 0; i < length; ++i)
    {
        snprintf(&hex[i * 2], 3, "%02X", payload[i]);
    }
    hex[length * 2] = '\0';
    ESP_LOGI(TAG, "%s: %s", prefix, hex);
}

static void log_downlink(const uint8_t *payload, size_t length, ttn_port_t port)
{
    ESP_LOGI(TAG, "Downlink ontvangen op port %u, %u bytes", port, (unsigned)length);
    log_hex_payload("Downlink payload (hex)", payload, length);
}

static void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static void init_spi_bus(void)
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = LORA_PIN_MOSI,
        .miso_io_num = LORA_PIN_MISO,
        .sclk_io_num = LORA_PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = 0,
        .flags = SPICOMMON_BUSFLAG_MASTER,
        .intr_flags = 0,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LORA_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO));
}

static void ensure_gpio_isr_service(void)
{
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(err);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Start LoRaWAN applicatie");

    init_nvs();
    init_spi_bus();
    ensure_gpio_isr_service();

    ttn_init();
    ttn_configure_pins(LORA_SPI_HOST, LORA_PIN_NSS, TTN_NOT_CONNECTED, LORA_PIN_RST, LORA_PIN_DIO0, LORA_PIN_DIO1);

    // The below line can be commented after the first run as the data is saved in NVS
    ttn_provision(DEV_EUI, APP_EUI, APP_KEY);

    // Register callback for received messages
    ttn_on_message(log_downlink);

    // Stel Europese band in (868 MHz). Voor andere regio's, zie README of menuconfig.
    ttn_set_data_rate(TTN_DR_EU868_SF7_BW125);
    ttn_set_max_tx_pow(14); // dBm

    ESP_LOGI(TAG, "Join procedure gestart...");
    ESP_LOGI(TAG, "Wachten op join (dit kan 30-60 seconden duren, soms langer)...");
    
    bool join_result = ttn_join();
    ESP_LOGI(TAG, "ttn_join() retourneerde: %s", join_result ? "TRUE" : "FALSE");
    
    if (!join_result)
    {
        ESP_LOGE(TAG, "Join mislukt, controleer keys en gateway bereik");
        ESP_LOGE(TAG, "Zonder succesvolle join kunnen we geen data verzenden!");
        return;
    }
    
    ESP_LOGI(TAG, "✓ Succesvol verbonden met TTN");
    ESP_LOGI(TAG, "Start met verzenden van uplinks...");

    uint32_t frame_counter = 0;

    while (true)
    {
        ESP_LOGI(TAG, "=== Voorbereiden uplink #%lu ===", (unsigned long)frame_counter);
        
        // Voorbeeld: Verzend meerdere waardes
        // Payload structuur (8 bytes totaal):
        // Byte 0-1: Frame counter (uint16_t)
        // Byte 2-3: Temperatuur in tienden graden (int16_t, bijv. 253 = 25.3°C)
        // Byte 4:   Luchtvochtigheid (uint8_t, 0-100%)
        // Byte 5-6: Lichtintensiteit in lux (uint16_t)
        // Byte 7:   Status byte (uint8_t, bitflags)
        
        // Voorbeeld waardes (vervang later met echte sensordata)
        int16_t temperature = 253;  // 25.3°C (in tienden graden)
        uint8_t humidity = 65;      // 65%
        uint16_t light = 1250;       // 1250 lux
        uint8_t status = 0x01;       // Status flags
        
        uint8_t payload[8];
        
        // Frame counter (2 bytes)
        payload[0] = (uint8_t)((frame_counter >> 8) & 0xFF);
        payload[1] = (uint8_t)(frame_counter & 0xFF);
        
        // Temperatuur (2 bytes, signed)
        payload[2] = (uint8_t)((temperature >> 8) & 0xFF);
        payload[3] = (uint8_t)(temperature & 0xFF);
        
        // Luchtvochtigheid (1 byte)
        payload[4] = humidity;
        
        // Lichtintensiteit (2 bytes)
        payload[5] = (uint8_t)((light >> 8) & 0xFF);
        payload[6] = (uint8_t)(light & 0xFF);
        
        // Status (1 byte)
        payload[7] = status;
        
        // Log de waardes
        ESP_LOGI(TAG, "Frame counter: %lu", (unsigned long)frame_counter);
        ESP_LOGI(TAG, "Temperatuur: %d (%.1f°C)", temperature, temperature / 10.0f);
        ESP_LOGI(TAG, "Luchtvochtigheid: %u%%", humidity);
        ESP_LOGI(TAG, "Lichtintensiteit: %u lux", light);
        ESP_LOGI(TAG, "Status: 0x%02X", status);
        
        log_hex_payload("Uplink payload (hex)", payload, sizeof(payload));
        ESP_LOGI(TAG, "Aanroepen ttn_transmit_message...");
        
        ttn_response_code_t result = ttn_transmit_message(payload, sizeof(payload), 1, false);
        
        ESP_LOGI(TAG, "ttn_transmit_message retourneerde: %d", (int)result);
        
        if (result == TTN_SUCCESSFUL_TRANSMISSION)
        {
            ESP_LOGI(TAG, "✓ Uplink #%lu succesvol verzonden", (unsigned long)frame_counter);
        }
        else if (result == TTN_SUCCESSFUL_RECEIVE)
        {
            ESP_LOGI(TAG, "✓ Uplink #%lu verzonden met downlink ontvangen", (unsigned long)frame_counter);
        }
        else
        {
            ESP_LOGW(TAG, "✗ Uplink #%lu mislukt (code %d)", (unsigned long)frame_counter, (int)result);
        }

        frame_counter++;
        ESP_LOGI(TAG, "Wachten %lu ms tot volgende uplink...", (unsigned long)UPLINK_INTERVAL_MS);
        vTaskDelay(pdMS_TO_TICKS(UPLINK_INTERVAL_MS));
    }
}