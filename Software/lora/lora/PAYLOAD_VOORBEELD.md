# Payload Voorbeelden

## Payload Structuur (8 bytes)

```
Byte 0-1: Frame counter (uint16_t, big-endian)
Byte 2-3: Temperatuur in tienden graden (int16_t, big-endian, signed)
Byte 4:   Luchtvochtigheid (uint8_t, 0-100%)
Byte 5-6: Lichtintensiteit in lux (uint16_t, big-endian)
Byte 7:   Status byte (uint8_t, bitflags)
```

## Voorbeeld 1: Eerste meting

**Waardes:**
- Frame counter: 0
- Temperatuur: 25.3°C (253 in tienden graden)
- Luchtvochtigheid: 65%
- Lichtintensiteit: 1250 lux
- Status: 0x01

**Hex payload:**
```
00 00 00 FD 41 04 E2 01
```

**Uitleg:**
- `00 00` = Frame counter 0 (uint16_t)
- `00 FD` = Temperatuur 253 (int16_t, = 25.3°C)
- `41` = Luchtvochtigheid 65 (0x41 = 65)
- `04 E2` = Lichtintensiteit 1250 (0x04E2 = 1250)
- `01` = Status 0x01

## Voorbeeld 2: Tweede meting

**Waardes:**
- Frame counter: 1
- Temperatuur: 22.5°C (225 in tienden graden)
- Luchtvochtigheid: 70%
- Lichtintensiteit: 800 lux
- Status: 0x03

**Hex payload:**
```
00 01 00 E1 46 03 20 03
```

**Uitleg:**
- `00 01` = Frame counter 1 (uint16_t)
- `00 E1` = Temperatuur 225 (int16_t, = 22.5°C)
- `46` = Luchtvochtigheid 70 (0x46 = 70)
- `03 20` = Lichtintensiteit 800 (0x0320 = 800)
- `03` = Status 0x03

## Voorbeeld 3: Negatieve temperatuur

**Waardes:**
- Frame counter: 10
- Temperatuur: -5.2°C (-52 in tienden graden)
- Luchtvochtigheid: 45%
- Lichtintensiteit: 2000 lux
- Status: 0x00

**Hex payload:**
```
00 0A FF CC 2D 07 D0 00
```

**Uitleg:**
- `00 0A` = Frame counter 10 (uint16_t)
- `FF CC` = Temperatuur -52 (int16_t, 0xFFCC = -52 in two's complement, = -5.2°C)
- `2D` = Luchtvochtigheid 45 (0x2D = 45)
- `07 D0` = Lichtintensiteit 2000 (0x07D0 = 2000)
- `00` = Status 0x00

## Gedecodeerde Output (TTN Console)

Na het toevoegen van de decoder in TTN, zie je:

```json
{
  "frame_counter": 0,
  "temperature": {
    "value": 25.3,
    "unit": "°C"
  },
  "humidity": {
    "value": 65,
    "unit": "%"
  },
  "light": {
    "value": 1250,
    "unit": "lux"
  },
  "status": {
    "bit0": true,
    "bit1": false,
    "bit2": false,
    "bit3": false,
    "bit4": false,
    "bit5": false,
    "bit6": false,
    "bit7": false,
    "raw": "0x01"
  }
}
```

## Test Payloads voor TTN Console

Je kunt deze hex strings gebruiken om de decoder te testen in de TTN console:

1. **Eerste meting:**
   ```
   000000FD4104E201
   ```

2. **Tweede meting:**
   ```
   000100E146032003
   ```

3. **Negatieve temperatuur:**
   ```
   000AFFCC2D07D000
   ```

4. **Hoge waardes:**
   ```
   00FF03E864138800
   ```
   (Frame: 255, Temp: 100.0°C, Humidity: 100%, Light: 5000 lux, Status: 0x00)

