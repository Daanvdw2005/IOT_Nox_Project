<!-- # NOX

## inhoud

- start project
- analyse

## analyse

We starten het project met het bekijken wat we allemaal moeten aansluiten en welke componenten we eventueel gaan vervangen. Elk component moet aan enkele vereisten voldoen. We werken aan dit project in 3 stadia:

1. appart
2. bakenverlichting
3. Drone

We willen ervoor zorgen dat we voor elk stadia zo weinig mogelijk moeten veranderen aan de opstelling. En dus zo weinig mogelijk andere componenten te gaan gebruiken.

### Walter module

De eerste stap die we gaan bekijken of we de raspberry pi's gaan vervangen door een esp32 module zoals de walter. De reden waarom we zouden kiezen voor de walter is dat deze een 4g module beschikt. -->

<!-- NIEUWE Documentatie -->

# NOX

## inhoud

- Het project
- Analyse
- Doel
- Technische basis
- Componenten
- Ap terra integratie
- Redesign en integratie
- Energie en teststrategie
- Huidige situatie
- Planning
- Verwachte resultaten

## Het project
Daan Van der Weken en Warre Van Rechem

Opdrachtgever: AP-Hogeschool
Opleiding: Electronica-ICT IoT
Lectoren: Mnr. Luyts, Mnr. Van Merode

Het NOx-project is een vervolg op het eerdere **Bakens V2-project**. Bij de bakens lag de focus op het slimmer maken van de technologie (zoals verlichting in de haven) om veiligheid te verhogen en onderhoud efficiënter te maken.

Bij NOx verschuift de focus naar het bouwen van een **mobiele sensoroplossing** die NOx-waardes kan meten. Deze data is belangrijk voor monitoring van de luchtkwaliteit in industriële omgevingen zoals havens.

## Analyse

We starten het project met het bekijken wat we allemaal moeten aansluiten en welke componenten we eventueel gaan vervangen. Elk component moet aan enkele vereisten voldoen. We werken aan dit project in 3 stadia:

1. Standalone:
   Een stabiele, zelfstandige oplossing voor NOx-metingen.
2. Stationair (bakenverlichting):
   Integratie in bestaande bakenverlichting.
3. Mobiel (drone):
   Ontwikkeling van een payload voor drone-gebaseerde metingen.

We willen ervoor zorgen dat we voor elk stadium zo weinig mogelijk moeten veranderen aan de opstelling, en dus zo weinig mogelijk andere componenten gaan gebruiken.

### Walter module

De eerste stap die we bekijken is of we de Raspberry Pi’s gaan vervangen door een **ESP32 module** zoals de Walter.
De reden waarom we zouden kiezen voor de Walter is dat deze beschikt over een ingebouwde **4G module** waarmee data direct kan doorgestuurd worden.

Na enkele testen hebben we besloten dat we geen gebruik gaan maken van de walter module. We werken vanaf nu voort met de FireBeetle.


### Fire Beetle

We zetten ons project verder met de esp fire beetle. Dit is een veel gebruikte esp die we op campus op voorhand hebben. Deze module is ruim beschreven op het internet waardoor het makkelijker is om deze aan te sturen.

## Doel

Het project heeft drie duidelijke pijlers:

1. **Meten van NOx-waardes**
   Een mobiele oplossing waarmee we luchtkwaliteit continu kunnen monitoren.
2. **Flexibiliteit in toepassingen**

   - **Stationaire oplossing**: integratie in de bestaande bakenverlichting.
   - **Mobiele oplossing**: een module die kan meereizen, bijvoorbeeld als payload op een drone.
3. **Optimaliseren van de energievoorziening**
   Voor zowel mobiele als stationaire systemen is een aangepaste energieoplossing nodig. Dit betekent herbekijken van de sspanningsbron en alternatieven uittesten.

Voor het eindproject (doel) hebben we een schema ontwikkeld dat hieronder zichtbaar is. Klik op de afbeelding om te vergroten:

[![Schema van het project](Afbeeldingen/Blokdiagram_doelstelling.drawio.png)](Afbeeldingen/Blokdiagram_doelstelling_XL.drawio.png)
## Technische basis

De originele versie van NOx was gebouwd op **ROS-technologie**. Voor de nieuwe versie schakelen we over naar **ESP32** met **ESP-IDF** als ontwikkelomgeving. Dit geeft ons meer flexibiliteit.

## Componenten

### 1. Quickspot Walter (ESP32 + 4G) **Niet meer in gebruik**

De **Walter-module** is een krachtige ESP32-gebaseerde controller die beschikt over een ingebouwde **4G-module**. Dit maakt het mogelijk om data rechtstreeks te verzenden naar een centrale server, zonder afhankelijk te zijn van lokale netwerken.

- **Functie in dit project**: centrale controller en communicatiehub.
- **Waarom gekozen**: energiezuinig, flexibel te programmeren (ESP-IDF) en geschikt voor zowel mobiele als stationaire toepassingen.

- **Reden stopzetting**: Na testen bleek de FireBeetle beter geschikt door beschikbaarheid en uitgebreide documentatie.

![Walter module](https://www.quickspot.io/images/walter-postcard.jpg)
---------------------------------------------------------------------

<small>bron: [quickspot.io](https://www.quickspot.io/)</small>

### 2. GY-GPS6MV2 (GPS-module)

De **GY-GPS6MV2** is een compacte en goedkope GPS-module die nauwkeurige locatiebepaling mogelijk maakt. Deze module gebruikt het NEO-6M GPS-chipset van u-blox.

- **Functie in dit project**: locatie koppelen aan elke NOx-meting.
- **Waarom gekozen**: lichtgewicht, laag energieverbruik en eenvoudig aan te sluiten via seriële communicatie.

![GY-GPS6MV2](https://cdn.bodanius.com/media/1/79b100556_gy-neo6mv2-gps-module_600x.webp)

---

<small>bron: [opencircuit.be](https://opencircuit.be/product/gy-neo6mv2-gps-module)</small>

### 3. TB600B (Gas Sensor Module voor NOx-meting)

De **TB600B-NO2** is een geavanceerde elektrochemische gas sensor module die state-of-the-art solid polymer sensor technologie combineert met een geoptimaliseerde printplaat voor nauwkeurige gasdetectie. Deze module detecteert specifiek stikstofdioxide (NO2), een sleutelcomponent van NOx-gassen, met een meetbereik van typisch 0-100 ppm (afhankelijk van de variant). De sensor genereert een proportioneel elektrisch signaal via elektrochemische reacties en biedt een gestandaardiseerde digitale UART-output voor eenvoudige integratie.

- **Functie in dit project**: Continue monitoring en meting van NOx-niveaus in de omgeving, voor emissie-analyse of luchtkwaliteitsbewaking. De data wordt verwerkt door de centrale controller voor real-time logging en alarmering.
- **Waarom gekozen**: Uitstekende gevoeligheid en lage kruisgevoeligheid met andere gassen; robuust ontwerp (verwachte levensduur >36 maanden); laag stroomverbruik (<1 mA) en compact formaat, ideaal voor embedded systemen in uitdagende (industriële) omgevingen. Geen vloeibare elektrolyten, dus onderhoudsarm en betrouwbaar.
- **Technische specs (kort)**:
  - Detectiebereik: 0-100 ppm NO2 (resolutie ~0.1 ppm).
  - Reactietijd: <30 seconden.
  - Voeding: 3.3-5V DC.
  - Communicatie: UART (9600 baud) voor directe interfacing met microcontrollers zoals ESP32 of Arduino.

![TB600B](https://ecsense.com/wp-content/uploads/2024/01/TB600B-UART-Smart-Gas-Sensor-Module_Image1_20230920.jpg)

---

<small>bron: [ecsense.com](https://ecsense.com/product/tb600b-tvoc-10-volatile-organic-compounds-gas-sensor-module/)</small>

### 4. Fire Beetle

De **Fire Beetle** is een microcontroller gebaseerd op de **ESP32-chip** , bekend om zijn krachtige prestaties, veelzijdigheid en ingebouwde WiFi- en Bluetooth-functionaliteit. De Fire Beetle is speciaal ontworpen voor IoT-toepassingen en maakt deel uit van de DFRobot-reeks van compacte ontwikkelborden.De Fire Beetle maakt gebruik van de esp32 chip. En is een krachtige controller

* functie: Het aansturen van randcomponenten
* Waarom gekozen: Ruime voorraad op de hogeschool. Makkelijk te programmeren via esp-idf.
  ![FireBeetle](https://m.media-amazon.com/images/I/71Ymtdf-fuL._UF1000,1000_QL80_.jpg)

---

<small>bron: [amazon.com](amazon.de/DFRobot-FireBeetle-kompatibel-Microcontroller-Entwicklungsboard-standard/dp/B08VNY821Y)</small>

### 5. LoRa Radio 868MHz TEL0125

We maken gebruik van deze module om via de fire beetle data te versturen via een lora netwerk te versturen. Het is een low power module gemaakt voor snel en gemakkelijk een transmissie te kunnen doen. Het maakt gebruik van long range modulatie en is dus ideaal om data over een grote afstand te verzenden met minimale energieconsumptie.

In het project maken we gebruik van deze module om in de stationaire fase data te gaan versturen naar ons ap terra platform. In latere fases zouden we naar andere technologien gaan kijken om de data te gaan zenden. Aangezien loraone bekend staat om lage zend tijd. En het ook zeer veel energie vraagt om elke meting te gaan doorsturen.

![TEL0125](https://github.com/DFRobot/TEL0125/blob/master/600px-TEL0125_WIKI_Cover.jpg?raw=true)

---

<small>bron: [dfrobot.com](https://wiki.dfrobot.com/FireBeetle_Covers_LoRa_Radio_868MHz_SKU_TEL0125)</small>

### 6. Zonnepaneel (SPM040201200)

- Het **SPM040201200** zonnepaneel levert 20W vermogen met een uitgangsstroom van 1,06A bij 18,5V.
- **Functie in dit project**: Energievoorziening voor de standalone en stationaire opstellingen, vooral voor langdurig gebruik in buitenomgevingen.
- **Waarom gekozen**: Voldoende vermogen om de FireBeetle, sensoren en communicatieapparatuur van stroom te voorzien, geschikt voor weerbestendige toepassingen.

![Victron](https://www.splitcharge.co.uk/wp-content/uploads/2022/06/SPM040201200-Victron-Energy-300x300.png)  
<small>bron: [splitcharge.co.uk](https://www.splitcharge.co.uk/product/victron-energy-20w-12v-mono-spm040201200-series-4a/?srsltid=AfmBOoqSyBVVAsSYU_M9W9vNs0R912W0VjO1F38gjK2i6Yjz0UWibSrl)</small>

### 7. Solar Power Manager (Waveshare)

- De **Waveshare Solar Power Manager** wordt gebruikt in combinatie met een lithium-polymeerbatterij om energie efficiënt te beheren.
- **Functie in dit project**: Regelt de energievoorziening vanuit het zonnepaneel en zorgt voor stabiele voeding naar de FireBeetle en andere componenten. Buffert energie in de batterij voor gebruik tijdens nachtelijke uren of bewolkte omstandigheden.
- **Waarom gekozen**: Betrouwbare energiebeheeroplossing, compatibel met lithium-polymeerbatterijen, en geschikt voor IoT-toepassingen.

![Solar Power Manager](https://www.waveshare.com/w/upload/thumb/c/cc/Solar-Power-Manager01.jpg/300px-Solar-Power-Manager01.jpg)  
<small>bron: [waveshare.com](https://www.waveshare.com/wiki/Solar_Power_Manager)</small>

### 8. PCB

- Een **custom PCB** wordt ontworpen om alle componenten (FireBeetle, GPS-module, gassensor, LoRa-module, en energiebeheer) efficiënt te integreren.
- **Functie in dit project**: Centrale hardwarebasis die alle componenten verbindt en zorgt voor een compacte, robuuste opstelling.
- **Waarom gekozen**: Verhoogt de betrouwbaarheid en vereenvoudigt de montage, vooral voor de stationaire en drone-fases.

![PCB_schematic](Afbeeldingen/image.png)
![PCB_3D](Afbeeldingen/image-1.png)

## Ap terra integratie

Een belangrijk onderdeel van de nieuwe versie van het NOx-project is de **integratie met AP Terra**.

**AP Terra** is een platform dat toelaat om apparaten en systemen centraal te monitoren en beheren. Voor dit project betekent dit:

- Alle NOx-metingen worden centraal doorgestuurd en weergegeven in AP Terra.
- Statusinformatie zoals batterij, communicatie en conditie van de sensor kan op afstand bekeken worden.
- Assetbeheer en onderhoudsplanning worden eenvoudiger doordat alle data op één plaats beschikbaar is.

Met deze integratie wordt het NOx-systeem niet enkel een sensoroplossing, maar ook een **volwaardige IoT-oplossing** die direct inzetbaar is bij de klant.

## Redesign en integratie

De integratie met de klant is gewijzigd, waardoor een **redesign** nodig is. Concreet:

- Het mobiele systeem moet betrouwbaar meetdata kunnen doorsturen.
- Voor de stationaire bakens willen we dezelfde NOx-functionaliteit kunnen toevoegen.
- Voor drones wordt een lichte en energiezuinige payload ontworpen, inclusief specifieke communicatie- en dataopslagmethoden.

## Energie en teststrategie

Een belangrijk aandachtspunt is de **energievoorziening**. Deze wordt kritisch geëvalueerd:

- Is ze geschikt voor langdurig gebruik?
- Kunnen we de efficiëntie verbeteren?

Daarnaast wordt er een **gestructureerde teststrategie** opgezet:

- Automatisch testen in plaats van enkel veldproeven.
- KaraDkteristieken opmeten en analyseren.
- Duurtestscenario’s om betrouwbaarheid te garanderen.

## Verwachte resultaten

Met NOx willen we een robuuste oplossing bouwen die zowel **stationair als mobiel** inzetbaar is. Het systeem moet in staat zijn om:

- Nauwkeurige NOx-metingen te doen.
- Data te koppelen aan locatie-informatie (GPS).
- Efficiënt en betrouwbaar te communiceren met de backend (via TB600B).
- Energiezuinig te functioneren, ook in uitdagende omstandigheden.
- Centraal beheerd en gemonitord te worden via **AP Terra**.

## Planning

De planning kan u terug vinden op onze project GitHub.
- Planning: [Planning](https://github.com/Daanvdw2005/IOT_Nox_Project/blob/main/Documentatie/planning_12_10_2025.md)
- GitHub: [GitHub](https://github.com/Daanvdw2005/IOT_Nox_Project.git)
