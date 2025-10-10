#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define UART_PORT_NUM UART_NUM_1
#define UART_BAUD_RATE 9600
#define UART_RX_PIN 16
#define UART_TX_PIN 17
#define UART_BUF_SIZE 1024
#define TASK_STACK_SIZE 4096 // Vergroot stackgrootte
#define TASK_PRIORITY 15 // Hogere prioriteit

static void uart_read_task(void *arg) {
    uint8_t data[UART_BUF_SIZE];
    char recv_buf[128];
    int recv_len = 0;

    printf("UART-taak gestart\n");

    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, data, sizeof(data), 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            printf("Ontvangen %d bytes: ", len);
            for (int i = 0; i < len; i++) {
                printf("%02x ", data[i]);
            }
            printf("\n");

            for (int i = 0; i < len; i++) {
                if (recv_len < sizeof(recv_buf) - 1) {
                    recv_buf[recv_len++] = (char)data[i];
                    if (data[i] == '\n') {
                        recv_buf[recv_len] = '\0';
                        printf("Ruwe data: %s\n", recv_buf);

                        char *conc_ptr = strstr(recv_buf, "CONC=");
                        if (conc_ptr) {
                            int conc_value = atoi(conc_ptr + 5);
                            printf("Gas concentratie: %d ppb\n", conc_value);
                        } else {
                            printf("Geen 'CONC=' gevonden in data\n");
                        }
                        recv_len = 0; // Reset buffer
                    }
                } else {
                    printf("Buffer overloop gedetecteerd!\n");
                    recv_len = 0; // Reset bij overloop
                }
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    // UART configuratie
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    printf("ESP32 TB600B lezer initialiseren...\n");
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0);

    vTaskDelay(5000 / portTICK_PERIOD_MS); // Wacht op sensor-opwarming
    printf("ESP32 TB600B lezer gestart. Wacht op data...\n");

    xTaskCreate(uart_read_task, "uart_read_task", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
}