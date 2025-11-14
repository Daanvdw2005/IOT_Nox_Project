#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ttn.h"

// ---------------- UART TB600B configuratie ----------------
#define UART_PORT_NUM      UART_NUM_1
#define UART_BAUD_RATE     9600
#define UART_RX_PIN        16
#define UART_TX_PIN        17
#define UART_BUF_SIZE      128
#define TASK_STACK_SIZE    4096
#define TASK_PRIORITY      10

// ---------------- LoRa configuratie ----------------
#define LORA_SPI_HOST SPI2_HOST
#define LORA_PIN_MISO GPIO_NUM_19
#define LORA_PIN_MOSI GPIO_NUM_23
#define LORA_PIN_SCK  GPIO_NUM_18
#define LORA_PIN_NSS  GPIO_NUM_27
#define LORA_PIN_RST  GPIO_NUM_25
#define LORA_PIN_DIO0 GPIO_NUM_26
#define LORA_PIN_DIO1 GPIO_NUM_9

static const char *TAG = "ESP32_TTN";

// TTN credentials (vul in vanuit TTN console)
static const char *const DEV_EUI = "70B3D57ED0074070";
static const char *const APP_EUI = "70B3D57ED0073702";
static const char *const APP_KEY = "B43689D3C3F09A6EA5C4A77A24B7D46D";

// Uplink interval
#define UPLINK_INTERVAL_MS (60 * 1000)

// ---------------- Globale sensorvariabele ----------------
volatile uint16_t latest_conc2 = 0;  // CO2 concentratie (ppm)

// ---------------- UART functies ----------------
void switch_to_auto_reporting_mode() {
    uint8_t cmd[] = {0xFF, 0x01, 0x78, 0x40, 0x00, 0x00, 0x00, 0x00, 0x47};
    uart_write_bytes(UART_PORT_NUM, cmd, sizeof(cmd));
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void parse_tb600b_data(uint8_t *data, size_t len) {
    if (len < 9) return;

    if (data[0] == 0xFF && data[1] == 0x86) {
        uint16_t conc2 = (data[2] << 8) | data[3];  // CO2 concentratie
        uint16_t checksum = data[8];

        uint16_t sum = 0;
        for (int i = 0; i < 8; i++) sum += data[i];
        uint8_t calculated_checksum = (0xFF - (sum & 0xFF)) & 0xFF;

        if (calculated_checksum == checksum) {
            latest_conc2 = conc2;  // update globale variabele
            printf("CO2 bijgewerkt: %d ppm\n", conc2);
        } else {
            printf("Checksum fout! Ontvangen: %02X, Bereken: %02X\n", checksum, calculated_checksum);
        }
    }
}

static void uart_read_task(void *arg) {
    uint8_t data[UART_BUF_SIZE];
    int pos = 0;

    printf("UART read task gestart...\n");

    // Sensor pas starten na TTN-connectie
    switch_to_auto_reporting_mode();

    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, &data[pos], 1, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            pos += len;
            if (pos >= 9) {
                parse_tb600b_data(data, pos);
                pos = 0;
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// ---------------- TTN / LoRa functies ----------------
static void log_downlink(const uint8_t *payload, size_t length, ttn_port_t port) {
    ESP_LOGI(TAG, "Downlink ontvangen op port %u, %u bytes", port, (unsigned)length);
    if (length > 0) {
        char hex[(length * 2) + 1];
        for (size_t i = 0; i < length; i++)
            snprintf(&hex[i * 2], 3, "%02X", payload[i]);
        hex[length * 2] = '\0';
        ESP_LOGI(TAG, "Downlink payload (hex): %s", hex);
    }
}

static void init_nvs(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static void init_spi_bus(void) {
    spi_bus_config_t bus_config = {
        .mosi_io_num = LORA_PIN_MOSI,
        .miso_io_num = LORA_PIN_MISO,
        .sclk_io_num = LORA_PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
        .flags = SPICOMMON_BUSFLAG_MASTER,
        .intr_flags = 0,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LORA_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO));
}

static void ensure_gpio_isr_service(void) {
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
        ESP_ERROR_CHECK(err);
}

// ---------------- Sensor naar TTN task ----------------
static void ttn_sensor_task(void *arg) {
    uint32_t frame_counter = 0;

    while (1) {
        uint8_t payload[2];
        payload[0] = (latest_conc2 >> 8) & 0xFF;
        payload[1] = latest_conc2 & 0xFF;

        ESP_LOGI(TAG, "Uplink #%lu: CO2 = %d ppm", frame_counter, latest_conc2);

        ttn_response_code_t result = ttn_transmit_message(payload, sizeof(payload), 1, false);

        if (result == TTN_SUCCESSFUL_TRANSMISSION) {
            ESP_LOGI(TAG, "✓ Uplink #%lu succesvol verzonden", frame_counter);
        } else if (result == TTN_SUCCESSFUL_RECEIVE) {
            ESP_LOGI(TAG, "✓ Uplink #%lu verzonden met downlink ontvangen", frame_counter);
        } else {
            ESP_LOGW(TAG, "✗ Uplink #%lu mislukt (code %d)", frame_counter, (int)result);
        }

        frame_counter++;
        vTaskDelay(pdMS_TO_TICKS(UPLINK_INTERVAL_MS)); // 1 minuut interval
    }
}

// ---------------- Hoofdprogramma ----------------
void app_main(void)
{
    ESP_LOGI(TAG, "Start LoRaWAN applicatie");

    // Init TTN
    init_nvs();
    init_spi_bus();
    ensure_gpio_isr_service();

    ttn_init();
    ttn_configure_pins(LORA_SPI_HOST, LORA_PIN_NSS, TTN_NOT_CONNECTED, LORA_PIN_RST, LORA_PIN_DIO0, LORA_PIN_DIO1);

    // TTN provisioning
    ttn_provision(DEV_EUI, APP_EUI, APP_KEY);

    // Callback voor downlink
    ttn_on_message(log_downlink);

    ttn_set_data_rate(TTN_DR_EU868_SF7_BW125);
    ttn_set_max_tx_pow(14);

    ESP_LOGI(TAG, "Join procedure gestart...");
    if (!ttn_join()) {
        ESP_LOGE(TAG, "Join mislukt, controleer keys en gateway");
        return;
    }
    ESP_LOGI(TAG, "✓ Succesvol verbonden met TTN");

    // UART pas initialiseren na succesvolle TTN join
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0));

    // Start UART en sensor naar TTN taken
    xTaskCreate(uart_read_task, "uart_read_task", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    xTaskCreate(ttn_sensor_task, "ttn_sensor_task", 4096, NULL, 5, NULL);
}
