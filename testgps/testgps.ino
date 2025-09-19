// gps_test.ino - Testcode voor GY-GPS6MV2 via Walter UART
// Doel: Lees NMEA-data en parseer $GPGGA voor positie
// Hardware: Walter (ESP32-S3) + GY-GPS6MV2
// UART: GPIO1 (TX), GPIO3 (RX), 9600 baud

#include <HardwareSerial.h>

// Definieer UART-pinnen (pas aan op basis van Walter pinout)
#define RX_PIN 3
#define TX_PIN 1
#define GPS_BAUD 9600

// UART-instantie (gebruik UART1 om conflicten met USB-serial te vermijden)
HardwareSerial gpsSerial(1);

// Definieer GPSData struct vóór gebruik in functies
struct GPSData {
  float latitude;
  float longitude;
  int fixQuality;
  String timestamp;
};

// Functieprototypes
bool parseNMEA(String sentence, GPSData &data);

void setup() {
  // Start seriële monitor voor debugging (USB)
  Serial.begin(115200);
  while (!Serial) {
    ; // Wacht op seriële verbinding
  }
  
  // Start UART voor GPS
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  
  Serial.println("GPS Test gestart. Wacht op NMEA-data van GY-GPS6MV2...");
  Serial.println("Plaats module buiten voor satelliet-fix (kan 1-5 min duren).");
}

bool parseNMEA(String sentence, GPSData &data) {
  // Controleer of het een $GPGGA-bericht is
  if (!sentence.startsWith("$GPGGA")) {
    return false;
  }

  // Splits de NMEA-regel op komma's
  int partsCount = 0;
  String parts[15];
  String current = "";
  
  for (char c : sentence) {
    if (c == ',') {
      parts[partsCount++] = current;
      current = "";
    } else {
      current += c;
    }
  }
  parts[partsCount] = current; // Laatste deel

  // Controleer of er genoeg velden zijn
  if (partsCount < 14) {
    return false;
  }

  // Parse relevante velden
  String lat = parts[2];  // Latitude (DDMM.MMMM)
  String ns = parts[3];   // N/S
  String lon = parts[4];  // Longitude (DDDMM.MMMM)
  String ew = parts[5];   // E/W
  String fix = parts[6];  // Fix kwaliteit
  String time = parts[1]; // UTC tijd (HHMMSS.ss)

  // Converteer naar decimale graden
  if (lat.length() > 0 && lon.length() > 0 && fix.length() > 0) {
    float latDeg = lat.substring(0, 2).toFloat();
    float latMin = lat.substring(2).toFloat();
    data.latitude = latDeg + latMin / 60.0;
    if (ns == "S") data.latitude = -data.latitude;

    float lonDeg = lon.substring(0, 3).toFloat();
    float lonMin = lon.substring(3).toFloat();
    data.longitude = lonDeg + lonMin / 60.0;
    if (ew == "W") data.longitude = -data.longitude;

    data.fixQuality = fix.toInt();
    data.timestamp = time;
    return true;
  }
  return false;
}

void loop() {
  // Lees beschikbare data van GPS
  if (gpsSerial.available()) {
    String sentence = gpsSerial.readStringUntil('\n');
    sentence.trim();
    
    if (sentence.length() > 0) {
      Serial.print("Ruwe NMEA: ");
      Serial.println(sentence);

      GPSData data;
      if (parseNMEA(sentence, data)) {
        Serial.println("*** GPS Fix gedetecteerd! ***");
        Serial.print("Latitude: ");
        Serial.print(data.latitude, 6);
        Serial.println(data.latitude >= 0 ? "°N" : "°S");
        Serial.print("Longitude: ");
        Serial.print(data.longitude, 6);
        Serial.println(data.longitude >= 0 ? "°E" : "°W");
        Serial.print("Fix kwaliteit: ");
        Serial.print(data.fixQuality);
        Serial.println(data.fixQuality == 1 ? " (GPS fix)" : " (Geen fix)");
        Serial.print("Tijd (UTC): ");
        Serial.println(data.timestamp);
        Serial.println("---");
      } else {
        Serial.println("Geen GPGGA-bericht, maar data ontvangen.");
      }
    }
  } else {
    Serial.print("."); // Wacht-indicator
    delay(1000);
  }

  delay(100); // Kleine delay om CPU te ontlasten
}