// index.js

const express = require('express');
const bodyParser = require('body-parser');
const { InfluxDB, Point } = require('@influxdata/influxdb-client');

const app = express();
const port = 3000; 

// Nodig om de JSON-payload van TTN te verwerken
app.use(bodyParser.json());


// INFLUXDB CONFIGURATIE - VERGEET NIET DIT TE VERVANGEN DOOR ENVIRONMENT VARIABELEN IN PRODUCTIE
const INFLUX_URL = 'http://freeforall.project-iot-ap.be:8086'; 
const INFLUX_TOKEN = 'ANnkMiMmGxcW98pl9M9YpeI2uuO521HI0tS-9sU9AtDGthPS3kv5ZuoQaL3Cl6XLBF_Iy8bqRLYWxAcb0GkaGQ=='; 
const INFLUX_BUCKET = 'Nox_database_28_11_2025'; 
const INFLUX_ORG = '5abb3979f50a4fa1'; 

// InfluxDB Client Setup
const client = new InfluxDB({ url: INFLUX_URL, token: INFLUX_TOKEN });
const writeApi = client.getWriteApi(INFLUX_ORG, INFLUX_BUCKET, 'ms'); // 'ms' is ingesteld als precisie


/**
 * De WebHook Handler: Ontvangt de POST-request van TTN op /ttn-ontvanger
 */
app.post('/ttn-ontvanger', async (req, res) => {
    
    const ttnData = req.body;
    console.log('\n--- TTN WebHook ontvangen ---');
    
    try {
        // 1. DATA EXTRACTIE (met Default Waarden voor ontbrekende GPS)
        const deviceId = ttnData.end_device_ids.device_id;
        const payload = ttnData.uplink_message.decoded_payload;

        if (!payload) {
            console.warn('Geen gedecodeerde payload gevonden. Controleer TTN decoder.');
            // Stuur status 204 (No Content) terug als er geen payload is, TTN zal dit niet als een fout zien
            return res.status(204).send("Geen payload.");
        }

        // --- Data-extractie en validatie ---

        // CO2: Gebruik 0 als default indien de waarde ontbreekt of null is
        const co2 = payload.co2_ppm || 0; 
        
        // Latitude & Longitude: Gebruik 0.0 als default indien de waarde ontbreekt of null is.
        // Dit voorkomt de fout 'invalid float value for field 'latitude': null' bij InfluxDB.
        const latitude = payload.lat === null || payload.lat === undefined ? 0.0 : payload.lat;
        const longitude = payload.lon === null || payload.lon === undefined ? 0.0 : payload.lon;
        
        // FIX: Gebruik de epoch timestamp en converteer naar milliseconden
        // Gebruik de huidige tijd als fallback als de timestamp ontbreekt in de payload
        const epochTimeSeconds = payload.timestamp || Math.floor(Date.now() / 1000); 
        const epochTimeMs = epochTimeSeconds * 1000; 
        
        console.log(`Verwerk data: Device=${deviceId}, CO2=${co2}, Lat=${latitude}, Lon=${longitude}, Epoch (ms)=${epochTimeMs}`);

        // 2. INFLUXDB POINT GENEREREN
        const point = new Point('sensor_metingen') 
            .tag('device_id', deviceId) 
            
            // De sensormetingen (Fields) - Nu veilig, aangezien null-waarden 0 of 0.0 zijn
            .intField('co2_ppm', co2) 
            .floatField('latitude', latitude) 
            .floatField('longitude', longitude) 
            
            // Gebruik de epoch timestamp in milliseconden
            .timestamp(epochTimeMs); 

        // 3. VERSTUREN
        writeApi.writePoint(point);
        await writeApi.flush(); 

        console.log(`Succesvol geschreven naar InfluxDB.`);
        return res.status(200).send("Data succesvol verwerkt");
        
    } catch (error) {
        // Log de fout, maar stuur een duidelijke 500 status terug naar TTN
        console.error("Fout bij verwerking of schrijven naar InfluxDB:", error.message || error);
        return res.status(500).send("Interne Serverfout.");
    }
});

// Start de server
app.listen(port, () => {
    console.log(`Middleware luistert op http://localhost:${port}`);
    // BELANGRIJK: Vergeet niet ngrok te starten om de server publiek te maken
});