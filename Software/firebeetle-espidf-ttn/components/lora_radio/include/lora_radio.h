#ifndef LORA_RADIO_H
#define LORA_RADIO_H

#include <stdint.h>

// LoRa radio configuration parameters
#define LORA_FREQUENCY 868E6  // Frequency in Hz
#define LORA_TX_POWER 14      // Transmission power in dBm

// Function declarations
void lora_radio_init(void);
void lora_radio_send(const uint8_t *data, uint8_t length);
void lora_radio_receive(uint8_t *buffer, uint8_t *length);

#endif // LORA_RADIO_H