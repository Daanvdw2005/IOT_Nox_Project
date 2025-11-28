// index.js

const express = require('express');
const bodyParser = require('body-parser');
const { InfluxDB, Point } = require('@influxdata/influxdb-client');

const app = express();
const port = 3000; 

// Nodig om de JSON-payload van TTN te verwerken
app.use(bodyParser.json());

// =======================================================
// STAP 1: CONFIGURATIE-INSTELLINGEN
// =======================================================
const INFLUX_URL = 'http://localhost:8086'; 
const INFLUX_TOKEN = 'qVAOiWIdgq9YVcWkwHLXCnDvtyyPAImpANQcZ6N2txsMVi7YKgOmLf8aRJjo_-KiptdgNbk9ibohuaiugxGr6Q=='; 
const INFLUX_BUCKET = 'Nox_database_28_11_25_Epoch'; 
const INFLUX_ORG = 'AP-Hogeschool'; 

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
        // 1. DATA EXTRACTIE (Gebruikt nu de platte payload structuur)
        const deviceId = ttnData.end_device_ids.device_id;
        const payload = ttnData.uplink_message.decoded_payload;

        if (!payload) {
            console.warn('Geen gedecodeerde payload gevonden. Controleer TTN decoder.');
            return res.status(204).send("Geen payload.");
        }

        // Nieuwe, platte velden uit de payload
        const co2 = payload.co2_ppm; 
        const latitude = payload.lat;
        const longitude = payload.lon;
        
        // FIX: Gebruik de epoch timestamp (in seconden) en converteer naar milliseconden (x 1000)
        // Dit is de meest betrouwbare methode voor InfluxDB.
        const epochTimeSeconds = payload.timestamp;
        const epochTimeMs = epochTimeSeconds * 1000; 
        
        console.log(`Verwerk data: Device=${deviceId}, CO2=${co2}, Lat=${latitude}, Epoch (ms)=${epochTimeMs}`);

        // 2. INFLUXDB POINT GENEREREN
        const point = new Point('sensor_metingen') 
            .tag('device_id', deviceId)                 
            
            // De sensormetingen (Fields)
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
    console.log(`Vergeet niet ngrok te starten in een andere terminal: ./ngrok http ${port}`);
});