
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "LoRa_FireBeetle"

// Pin definitions (based on document)
#define PIN_SCK  GPIO_NUM_18
#define PIN_MISO GPIO_NUM_19
#define PIN_MOSI GPIO_NUM_23
#define PIN_CS   GPIO_NUM_4  // CS on D4
#define PIN_RST  GPIO_NUM_2  // RESET on D2
#define PIN_LED  GPIO_NUM_2  // Assuming LED_BUILTIN is GPIO2 (adjust as needed)

// SX127x registers (based on SX1276/77/78 datasheet)
#define REG_OP_MODE         0x01
#define REG_FR_MSB          0x06
#define REG_FR_MID          0x07
#define REG_FR_LSB          0x08
#define REG_PA_CONFIG       0x09
#define REG_FIFO            0x00
#define REG_FIFO_ADDR_PTR   0x0D
#define REG_FIFO_TX_BASE    0x0E
#define REG_FIFO_RX_BASE    0x0F
#define REG_IRQ_FLAGS       0x12
#define REG_RX_NB_BYTES     0x13
#define REG_MODEM_CONFIG_1  0x1D
#define REG_MODEM_CONFIG_2  0x1E
#define REG_PAYLOAD_LENGTH  0x22
#define REG_FIFO_RX_CURRENT 0x10
#define REG_RSSI_VALUE      0x1B

// LoRa configuration
#define LORA_FREQ 915000000 // 915MHz
#define LORA_BW 125000      // Bandwidth 125kHz (7: 125kHz)
#define LORA_SF 7           // Spreading factor
#define LORA_CR 5           // Coding rate 4/5
#define LORA_POWER 0xFF     // Max power 20dBm (PA_BOOST)

spi_device_handle_t spi;

static void init_gpio() {
    gpio_set_direction(PIN_CS, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_CS, 1); // CSಸ

    // Perform hardware reset
    gpio_set_level(PIN_RST, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(PIN_RST, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

static void init_spi() {
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_MISO,
        .mosi_io_num = PIN_MOSI,
        .sclk_io_num = PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000, // 1MHz
        .mode = 0,                 // SPI mode 0
        .spics_io_num = PIN_CS,
        .queue_size = 7,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));
}

static uint8_t sx127x_read_reg(uint8_t reg) {
    uint8_t tx_buf[2] = { reg & 0x7F, 0 };
    uint8_t rx_buf[2] = { 0 };
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
    return rx_buf[1];
}

static void sx127x_write_reg(uint8_t reg, uint8_t val) {
    uint8_t tx_buf[2] = { reg | 0x80, val };
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_buf,
    };
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
}

static void sx127x_init_lora() {
    init_gpio();
    
    // Set LoRa mode
    sx127x_write_reg(REG_OP_MODE, 0x80); // Sleep mode, LoRa
    vTaskDelay(10 / portTICK_PERIOD_MS);
    sx127x_write_reg(REG_OP_MODE, 0x81); // Standby mode, LoRa

    // Set frequency (915MHz)
    uint64_t frf = ((uint64_t)LORA_FREQ << 19) / 32000000;
    sx127x_write_reg(REG_FR_MSB, (uint8_t)(frf >> 16));
    sx127x_write_reg(REG_FR_MID, (uint8_t)(frf >> 8));
    sx127x_write_reg(REG_FR_LSB, (uint8_t)frf);

    // Set bandwidth, spreading factor, coding rate
    sx127x_write_reg(REG_MODEM_CONFIG_1, (7 << 4) | (LORA_CR - 4) << 1);
    sx127x_write_reg(REG_MODEM_CONFIG_2, (LORA_SF << 4) | 0x04); // CRC enabled

    // Set power (20dBm, PA_BOOST)
    sx127x_write_reg(REG_PA_CONFIG, 0x80 | (LORA_POWER & 0x0F));

    // Set FIFO addresses
    sx127x_write_reg(REG_FIFO_TX_BASE, 0x00);
    sx127x_write_reg(REG_FIFO_RX_BASE, 0x00);

    ESP_LOGI(TAG, "LoRa Initialized");
}

static void sx127x_transmit(uint8_t *data, size_t len) {
    sx127x_write_reg(REG_OP_MODE, 0x81); // Standby
    sx127x_write_reg(REG_FIFO_ADDR_PTR, 0x00);
    
    for (size_t i = 0; i < len; i++) {
        sx127x_write_reg(REG_FIFO, data[i]);
    }
    
    sx127x_write_reg(REG_PAYLOAD_LENGTH, len);
    sx127x_write_reg(REG_OP_MODE, 0x83); // TX mode
    
    while (sx127x_read_reg(REG_OP_MODE) == 0x83) {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    
    sx127x_write_reg(REG_IRQ_FLAGS, 0xFF); // Clear interrupts
}

static bool sx127x_receive(uint8_t *buf, size_t max_len, int *rssi, uint32_t timeout_ms) {
    sx127x_write_reg(REG_OP_MODE, 0x85); // RXCONTINUOUS
    uint32_t start = xTaskGetTickCount();
    
    while (xTaskGetTickCount() - start < pdMS_TO_TICKS(timeout_ms)) {
        if (sx127x_read_reg(REG_IRQ_FLAGS) & 0x40) { // RxDone
            uint8_t len = sx127x_read_reg(REG_RX_NB_BYTES);
            sx127x_write_reg(REG_FIFO_ADDR_PTR, sx127x_read_reg(REG_FIFO_RX_CURRENT));
            
            for (uint8_t i = 0; i < len && i < max_len; i++) {
                buf[i] = sx127x_read_reg(REG_FIFO);
            }
            
            *rssi = -157 + sx127x_read_reg(REG_RSSI_VALUE); // Approximate RSSI
            sx127x_write_reg(REG_IRQ_FLAGS, 0xFF); // Clear interrupts
            return true;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    
    return false;
}

static void lora_task(void *pvParameters) {
    uint8_t counter = 0;
    uint8_t rx_buf[32];
    char tx_buf[32];
    int rssi;

    while (1) {
        // Receive
        if (sx127x_receive(rx_buf, sizeof(rx_buf), &rssi, 1000)) {
            ESP_LOGI(TAG, "Received: %.*s, RSSI: %d", rx_buf[0], rx_buf + 1, rssi);
            gpio_set_level(PIN_LED, !gpio_get_level(PIN_LED)); // Toggle LED
        }

        // Send every 5 seconds
        snprintf(tx_buf, sizeof(tx_buf), "Hello from Board %d!", counter++);
        ESP_LOGI(TAG, "Sending: %s", tx_buf);
        sx127x_transmit((uint8_t *)tx_buf, strlen(tx_buf) + 1);
        
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    init_spi();
    sx127x_init_lora();
    xTaskCreate(lora_task, "lora_task", 4096, NULL, 5, NULL);
}
