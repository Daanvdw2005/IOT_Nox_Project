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

// --- LoRa radio pin mapping for FireBeetle ESP32 + LoRa cover ----------------
#define LORA_SPI_HOST SPI2_HOST
#define LORA_PIN_MISO GPIO_NUM_19
#define LORA_PIN_MOSI GPIO_NUM_23
#define LORA_PIN_SCK GPIO_NUM_18
#define LORA_PIN_NSS GPIO_NUM_27   // CS / NSS
#define LORA_PIN_RST GPIO_NUM_25   // RST
#define LORA_PIN_DIO0 GPIO_NUM_26  // DIO0
#define LORA_PIN_DIO1 GPIO_NUM_9   // DIO1 (some boards leave this unconnected)

// --- TTN credentials (replace with values from the TTN console) --------------
static const char *const DEV_EUI = "0000000000000000";
static const char *const APP_EUI = "0000000000000000";
static const char *const APP_KEY = "00000000000000000000000000000000";

// Delay between uplinks
#define UPLINK_INTERVAL_MS (60 * 1000)

static void log_downlink(const uint8_t *payload, size_t length, ttn_port_t port)
{
    ESP_LOGI(TAG, "Downlink ontvangen op port %u, %u bytes", port, (unsigned)length);
    if (length > 0)
    {
        char hex[(length * 2) + 1];
        for (size_t i = 0; i < length; ++i)
        {
            snprintf(&hex[i * 2], 3, "%02X", payload[i]);
        }
        hex[length * 2] = '\0';
        ESP_LOGI(TAG, "Payload (hex): %s", hex);
    }
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
    ttn_on_message(log_downlink);
    ttn_configure_pins(LORA_SPI_HOST, LORA_PIN_NSS, TTN_NOT_CONNECTED, LORA_PIN_RST, LORA_PIN_DIO0, LORA_PIN_DIO1);

    if (!ttn_provision(DEV_EUI, APP_EUI, APP_KEY))
    {
        ESP_LOGE(TAG, "Provisioning mislukt, controleer EEPROM-toegang en keys");
        return;
    }

    // Stel Europese band in (868 MHz). Voor andere regio's, zie README of menuconfig.
    ttn_set_data_rate(TTN_DR_EU868_SF7_BW125);
    ttn_set_max_tx_pow(14); // dBm

    ESP_LOGI(TAG, "Join procedure gestart...");
    if (!ttn_join())
    {
        ESP_LOGE(TAG, "Join mislukt, controleer keys en gateway bereik");
        return;
    }
    ESP_LOGI(TAG, "Succesvol verbonden met TTN");

    uint32_t frame_counter = 0;

    while (true)
    {
        uint8_t payload[2] = {
            (uint8_t)((frame_counter >> 8) & 0xFF),
            (uint8_t)(frame_counter & 0xFF),
        };

        ttn_response_code_t result = ttn_transmit_message(payload, sizeof(payload), 1, false);
        if (result == TTN_SUCCESSFUL_TRANSMISSION)
        {
            ESP_LOGI(TAG, "Uplink #%lu verzonden", (unsigned long)frame_counter);
        }
        else if (result == TTN_SUCCESSFUL_RECEIVE)
        {
            ESP_LOGI(TAG, "Uplink #%lu met bevestigde downlink", (unsigned long)frame_counter);
        }
        else
        {
            ESP_LOGW(TAG, "Uplink #%lu mislukt (code %d)", (unsigned long)frame_counter, (int)result);
        }

        frame_counter++;
        vTaskDelay(pdMS_TO_TICKS(UPLINK_INTERVAL_MS));
    }
}