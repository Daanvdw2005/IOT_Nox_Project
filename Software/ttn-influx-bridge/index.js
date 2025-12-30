const express = require('express');
const bodyParser = require('body-parser');
const { InfluxDB, Point } = require('@influxdata/influxdb-client');

const app = express();
const port = 3000; 

app.use(bodyParser.json());

// --- INFLUXDB CONFIGURATIE ---
const INFLUX_URL = 'http://freeforall.project-iot-ap.be:8086'; 
const INFLUX_TOKEN = 'ANnkMiMmGxcW98pl9M9YpeI2uuO521HI0tS-9sU9AtDGthPS3kv5ZuoQaL3Cl6XLBF_Iy8bqRLYWxAcb0GkaGQ=='; 
const INFLUX_BUCKET = 'Nox_database_28_11_2025'; 
const INFLUX_ORG = '5abb3979f50a4fa1'; 

const client = new InfluxDB({ url: INFLUX_URL, token: INFLUX_TOKEN });
const writeApi = client.getWriteApi(INFLUX_ORG, INFLUX_BUCKET, 'ms'); 

app.post('/ttn-ontvanger', async (req, res) => {
    const ttnData = req.body;
    console.log('\n--- TTN WebHook ontvangen ---');
    
    try {
        if (!ttnData.uplink_message || !ttnData.uplink_message.decoded_payload) {
            console.warn('Geen payload gevonden. Is de decoder in TTN correct ingesteld?');
            return res.status(200).send("Geen data om te verwerken.");
        }

        const deviceId = ttnData.end_device_ids.device_id;
        const payload = ttnData.uplink_message.decoded_payload;
        
        const networkTime = ttnData.uplink_message.received_at 
                            ? new Date(ttnData.uplink_message.received_at) 
                            : new Date();

        const nox = payload.nox !== undefined ? payload.nox : 0;
        const batteryV = payload.battery_v !== undefined ? payload.battery_v : 0;
        const gpsValid = payload.gps_valid === true;

        let latitude = 0.0;
        let longitude = 0.0;

        if (gpsValid) {
            latitude = payload.latitude;
            longitude = payload.longitude;
        }

        console.log(`Verwerk data: Device=${deviceId}, Tijd=${networkTime.toISOString()}, NOx=${nox}, Lat=${latitude}, Lon=${longitude}`);

        // 3. INFLUXDB POINT MAKEN
        const point = new Point('sensor_metingen') 
            .tag('device_id', deviceId) 
            .intField('nox', nox)
            .floatField('battery_v', batteryV)
            .floatField('latitude', latitude)
            .floatField('longitude', longitude)
            .booleanField('gps_valid', gpsValid)
            .timestamp(networkTime); 

        // 4. VERSTUREN
        writeApi.writePoint(point);
        await writeApi.flush(); 

        console.log(`Succesvol geschreven naar InfluxDB (Timestamp: ${networkTime.toISOString()}).`);
        return res.status(200).send("Data succesvol verwerkt");
        
    } catch (error) {
        console.error("Fout bij verwerking:", error.message || error);
        return res.status(500).send("Fout bij opslaan.");
    }
});

app.listen(port, () => {
    console.log(`Server luistert op http://localhost:${port}`);
});