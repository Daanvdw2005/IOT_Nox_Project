#include <TinyGPSPlus.h>  // TinyGPS++ bibliotheek voor GPS-parsing
#include <HardwareSerial.h>  // HardwareSerial voor UART-communicatie

// Definieer pinnen voor GPS-communicatie
#define RX_PIN 44  // RX-pin voor GY-GPS6MV2
#define TX_PIN 43  // TX-pin voor GY-GPS6MV2

// HardwareSerial voor GPS-communicatie
HardwareSerial GPSSerial(1);

// TinyGPS++ object
TinyGPSPlus gps;

// Standaardcoördinaten (als er geen fix is)
float lat = 28.5458;
float lon = 77.1703;

void setup() {
  // Start seriële communicatie voor debugging
  Serial.begin(115200);
  while (!Serial);  // Wacht tot seriële poort klaar is
  Serial.println("GY-GPS6MV2 Test Starten...");

  // Start HardwareSerial voor GPS op pinnen 44 (RX) en 43 (TX)
  GPSSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  Serial.println("GPS Serial gestart op 9600 baud");
}

void loop() {
  // Lees en parseer GPS-gegevens
  while (GPSSerial.available() > 0) {
    if (gps.encode(GPSSerial.read())) {  // Parseer NMEA-data
      if (gps.location.isValid()) {  // Controleer of er een geldige fix is
        lat = gps.location.lat();
        lon = gps.location.lng();

        // Toon positie
        Serial.print("Positie: ");
        Serial.print("Breedtegraad: ");
        Serial.print(lat, 6);
        Serial.print(" ; Lengtegraad: ");
        Serial.println(lon, 6);
      } else {
        Serial.println("Geen GPS-fix...");
      }
    }
  }

  // Optioneel: Print als semicolon-gescheiden string
  Serial.print(String(lat, 6));
  Serial.print(";");
  Serial.println(String(lon, 6));

  delay(1000);  // Update elke seconde
}