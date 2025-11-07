#include "ttn.h"
#include "lora_radio.h"
#include <string.h>
#include <stdio.h>

#define APP_EUI "your_app_eui_here"
#define APP_KEY "your_app_key_here"
#define DEVICE_EUI "your_device_eui_here"

static void ttn_send_data(const char *data) {
    // Prepare the data to be sent
    lora_radio_send(data, strlen(data));
}

void ttn_connect() {
    // Initialize LoRa radio
    lora_radio_init();

    // Connect to The Things Network
    lora_radio_join(APP_EUI, APP_KEY, DEVICE_EUI);
}

void ttn_transmit(const char *data) {
    if (data != NULL) {
        ttn_send_data(data);
    }
}