// TTN Payload Decoder voor LoRaWAN device
// Plak deze functie in de TTN Console: Application → Payload formatters → Uplink decoder

function decodeUplink(input) {
  const bytes = input.bytes;
  
  // Controleer payload lengte
  if (bytes.length < 8) {
    return {
      errors: ['Payload te kort, verwacht minimaal 8 bytes']
    };
  }
  
  // Decode payload volgens structuur:
  // Byte 0-1: Frame counter (uint16_t, big-endian)
  // Byte 2-3: Temperatuur in tienden graden (int16_t, big-endian)
  // Byte 4:   Luchtvochtigheid (uint8_t, 0-100%)
  // Byte 5-6: Lichtintensiteit in lux (uint16_t, big-endian)
  // Byte 7:   Status byte (uint8_t, bitflags)
  
  const frame_counter = (bytes[0] << 8) | bytes[1];
  
  // Temperatuur: signed int16_t (big-endian)
  let temperature_raw = (bytes[2] << 8) | bytes[3];
  if (temperature_raw > 32767) {
    temperature_raw = temperature_raw - 65536; // Convert to signed
  }
  const temperature = temperature_raw / 10.0; // In graden Celsius
  
  const humidity = bytes[4]; // Percentage
  
  const light = (bytes[5] << 8) | bytes[6]; // Lux
  
  const status = bytes[7];
  
  // Decode status flags (voorbeeld)
  const status_flags = {
    bit0: (status & 0x01) !== 0,
    bit1: (status & 0x02) !== 0,
    bit2: (status & 0x04) !== 0,
    bit3: (status & 0x08) !== 0,
    bit4: (status & 0x10) !== 0,
    bit5: (status & 0x20) !== 0,
    bit6: (status & 0x40) !== 0,
    bit7: (status & 0x80) !== 0,
    raw: '0x' + status.toString(16).padStart(2, '0')
  };
  
  return {
    data: {
      frame_counter: frame_counter,
      temperature: {
        value: temperature,
        unit: '°C'
      },
      humidity: {
        value: humidity,
        unit: '%'
      },
      light: {
        value: light,
        unit: 'lux'
      },
      status: status_flags
    }
  };
}

