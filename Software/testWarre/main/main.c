#include <stdio.h>      // Voor printf (vervangt Serial)
#include <stdint.h>     // Voor uint8_t en andere typen
#include "DFRobot_LoRa.h" // Aangenomen dat deze bibliotheek beschikbaar is

// Definieer constante voor LED (vervangt LED_BUILTIN)
#define LED_PORT  PORTB // Voorbeeld: poort voor LED, platformafhankelijk
#define LED_PIN   5     // Voorbeeld: pin 5, pas aan voor jouw hardware

// Globale variabelen
DFRobot_LoRa lora;      // LoRa-object
uint8_t len;            // Lengte van ontvangen data
uint8_t rxBuf[32];      // Buffer voor ontvangen data
static uint8_t i = 0;   // Voor LED-toggle

// Vervangt Arduino's Serial.println en Serial.print
void print_string(const char* str) {
    printf("%s\n", str); // Simuleert Serial.println
}

void print_buffer(uint8_t* buf, uint8_t length) {
    for (uint8_t i = 0; i < length; i++) {
        printf("%c", buf[i]); // Simuleert Serial.write
    }
    printf("\n");
}

void print_rssi(int rssi) {
    printf("with RSSI %d\n", rssi);
}

// Vervangt Arduino's pinMode en digitalWrite
void setup_gpio() {
    // Stel LED-pin in als uitvoer (platformspecifiek)
    // Voorbeeld voor AVR-microcontroller:
    // DDRB |= (1 << LED_PIN); // Zet pin als uitvoer
    printf("GPIO configured for LED\n"); // Placeholder
}

void toggle_led(uint8_t state) {
    // Schrijf naar LED-pin (platformspecifiek)
    // Voorbeeld: PORTB = (state ? (PORTB | (1 << LED_PIN)) : (PORTB & ~(1 << LED_PIN)));
    printf("LED state: %d\n", state); // Placeholder
}

// Setup-functie (vervangt Arduino setup)
void setup() {
    // Initialiseer seriële communicatie (vervangt Serial.begin)
    // In standaard C afhankelijk van platform, hier gesimuleerd
    printf("Receiver Test\n");

    // Configureer LED-pin
    setup_gpio();

    // Initialiseer LoRa-module
    if (!lora.init()) {
        print_string("Starting LoRa failed!");
        while (1); // Oneindige lus bij fout
    }

    lora.rxInit(); // Stel LoRa in voor ontvangst
}

// Loop-functie (vervangt Arduino loop)
void loop() {
    if (lora.waitIrq()) { // Wacht op RXDONE-interrupt
        lora.clearIRQFlags(); // Wis interrupt-vlaggen
        len = lora.receivePackage(rxBuf); // Ontvang data
        print_buffer(rxBuf, len); // Print ontvangen data
        lora.rxInit(); // Herinitialiseer voor nieuwe ontvangst

        // Print RSSI van pakket
        print_rssi(lora.readRSSI());

        // Toggle LED
        i = ~i;
        toggle_led(i);
    }
}

// Hoofdprogramma
int main(void) {
    setup(); // Voer setup uit
    while (1) {
        loop(); // Voer loop herhaaldelijk uit
    }
    return 0; // Wordt nooit bereikt
}