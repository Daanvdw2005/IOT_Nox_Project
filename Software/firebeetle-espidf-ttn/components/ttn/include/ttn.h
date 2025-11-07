#ifndef TTN_H
#define TTN_H

#include <stdint.h>

// TTN configuration
#define TTN_APP_EUI "your_app_eui_here"
#define TTN_APP_KEY "your_app_key_here"

// Function declarations
void ttn_init(void);
void ttn_send_data(uint8_t *data, uint16_t length);

#endif // TTN_H