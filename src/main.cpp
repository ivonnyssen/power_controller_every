/*
 Power Controller on a web page
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Relay module is attached to digital pins 5, 6, 7, 8
 */

#include <SPI.h>
#include <Ethernet.h>
#include "page.h"
#include <TimeLib.h>
#define TINY_BME280_I2C
#include <TinyBME280.h>
#include <Wire.h>
#include "bms.h"

#define GET 0
#define POST 1
#define UNSUPPORTED 2

#define OFF 0
#define ON 1
#define CYCLE 2

#define NUM_PORTS 4

#define DEBUG false

typedef struct Request{
    int type;
    String url;
    long powerPort;
    long command;
} Request;

typedef struct SensorData{
    time_t readoutTime;
    float pressure;
    float temperature;
    float humidity;
} SensorData;

Request parseRequest(EthernetClient client);

void readAndLogRequestLines(EthernetClient client);

void printWebPage(EthernetClient client, const String &url, int type);

void sendNtpPacket(const char * address);

void handleHttpRequest(EthernetClient &client);

time_t getNtpTime();

void measureAndLogSensors(time_t &now);

void printBmsFaults(EthernetClient &client);

void printCellVoltages(EthernetClient &client);

void printBmsStates(EthernetClient &client);

void printSensorsJson(EthernetClient &client);

void printIndexPage(EthernetClient &client);

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

//NTP stuff
#define 	NTP_OFFSET   3155673600
#define 	UNIX_OFFSET   946684800
const char ntpServer[] = "time.nist.gov";
const int ntpPacketSize = 48;
uint8_t packetBuffer[ntpPacketSize];
const int timeZone = -8; //PST
unsigned int localPort = 8888;
EthernetUDP Udp;

//sensor data
const int numSensorRecords = 24 * 4;
SensorData sensorData[numSensorRecords];
time_t lastSensorLogTime;

//variables for states
bool ports[4] {false,false,false,false};
#define BASE_PORT_PIN 3

//BME280
tiny::BME280 bme;

//Serial BMS connection
BMS bms;
time_t lastBmsCheckTime;

#ifndef UNIT_TEST
void setup() {
    // Open serial communications and wait for port to open:
    Serial.begin(9600);

    // start the Ethernet connection and the server:
    EthernetClass::begin(mac);
    server.begin();
#if DEBUG
    Serial.println(EthernetClass::localIP());
#endif
    //UDP for NTP
    Udp.begin(localPort);
    delay(2000);
    setSyncProvider(getNtpTime);
    setSyncInterval(SECS_PER_HOUR);
#if DEBUG
    if(timeStatus() != timeSet) Serial.println("Unable to sync with NTP");
    Serial.println(now());
#endif
    //relay module setup
    pinMode(BASE_PORT_PIN, OUTPUT);
    pinMode(BASE_PORT_PIN  + 1, OUTPUT);
    pinMode(BASE_PORT_PIN + 2, OUTPUT);
    pinMode(BASE_PORT_PIN +3, OUTPUT);
    for(int i = BASE_PORT_PIN; i < BASE_PORT_PIN + NUM_PORTS; i++){
        ports[i - BASE_PORT_PIN] = digitalRead(i) != HIGH;
    }

    //initialize sensor stuff below
    memset(&sensorData, 0, sizeof(SensorData));

    //bme280
    Wire.begin();
    bme.begin();

    //BMS
    bms.begin(&Serial1);
}

void  loop() {

    // listen for incoming clients
    EthernetClient client = server.available();
    if (client) {
        handleHttpRequest(client);
    }

    time_t seconds = now();
    if(seconds % 900 == 0 && seconds != lastSensorLogTime) {
        measureAndLogSensors(seconds);
        lastSensorLogTime = seconds;
    }

    if(seconds % 30 == 0 && seconds != lastBmsCheckTime){
        bms.poll();
        lastBmsCheckTime = seconds;
    }

    if(seconds % SECS_PER_DAY == 0){
        bms.clear24Values();
        bms.clearFaultCounts();
    }
}
#endif

void measureAndLogSensors(time_t &now) {
    for(int i = 0; i < numSensorRecords - 1; i++){
        sensorData[i] = sensorData[i + 1];
    }
    sensorData[numSensorRecords - 1] = {now, bme.readFixedPressure() / 100.0, bme.readFixedTempC() / 100.0, bme.readFixedHumidity() / 1000.0}; // NOLINT(cppcoreguidelines-narrowing-conversions)

#if DEBUG
    char buffer[32] = {0};
    sprintf(buffer, "%d, %s, %s, %s", sensorData[numSensorRecords - 1].readoutTime, String(sensorData[numSensorRecords - 1].pressure).c_str(),
            String(sensorData[numSensorRecords - 1].temperature).c_str(), String(sensorData[numSensorRecords - 1].humidity).c_str());
    Serial.println(buffer);
#endif
}

void handleHttpRequest(EthernetClient &client) {
    if (client.available()) {
        Request request = parseRequest(client);
        switch (request.type) {
            case GET:
                printWebPage(client, request.url, GET);
                break;
            case POST:
                switch(request.command){
                    case OFF:
                        ports[request.powerPort] = false;
                        digitalWrite(request.powerPort + BASE_PORT_PIN, HIGH);
                        break;
                    case ON:
                        ports[request.powerPort] = true;
                        digitalWrite(request.powerPort + BASE_PORT_PIN, LOW);
                        break;
                    case CYCLE:
                        ports[request.powerPort] = false;
                        digitalWrite(request.powerPort + BASE_PORT_PIN, HIGH);
                        delay(1000);
                        ports[request.powerPort] = true;
                        digitalWrite(request.powerPort + BASE_PORT_PIN, LOW);
                        break;
                    default:
                        break;
                }
                printWebPage(client, "/", POST);
            default:
                break;
        }
    }

    delay(10);
    client.stop();
}

Request parseRequest(EthernetClient client) {
    Request result{};

    String s = client.readStringUntil('\n');
#if DEBUG
    Serial.println(s);
#endif
    if(s.startsWith("GET")){
        result.type = GET;
        result.url = s.substring(4, s.lastIndexOf(' '));
        readAndLogRequestLines(client);
    } else if(s.startsWith("POST")){
        result.type = POST;
        readAndLogRequestLines(client);
        if(client.available()){
            s = client.readStringUntil('\n');
            if(s.startsWith("power")){
                result.powerPort=s.substring(5,6).toInt();
                result.command=s.substring(7,8).toInt();
            }
        }
    } else {
        result.type = UNSUPPORTED;
    }
#if DEBUG
    Serial.println(result.url);
#endif
    return result;
}

void readAndLogRequestLines(EthernetClient client) {
    while (client.available()) {
        String s = client.readStringUntil('\n');
#if DEBUG
        Serial.println(s);
#endif
        if(s.equals(String('\r'))){
            break;
        }
    }
}

void printWebPage(EthernetClient client, const String &url, const int type) {
    //print header
    if (type == POST) {
        client.println("HTTP/1.1 303 See Other");
        char buffer[64] = {0};
        sprintf(buffer, "Location: http://%d.%d.%d.%d%s",EthernetClass::localIP()[0],EthernetClass::localIP()[1],
                EthernetClass::localIP()[2],EthernetClass::localIP()[3],url.c_str());
        Serial.println(buffer);
        client.println(buffer);
    } else {
        client.println("HTTP/1.1 200 OK");
        if(url.equals("/")){
            char buffer[64] = {0};
            sprintf(buffer, F("Refresh: 450; url=http://%d.%d.%d.%d%s"),EthernetClass::localIP()[0],
                    EthernetClass::localIP()[1],EthernetClass::localIP()[2],EthernetClass::localIP()[3],url.c_str());
            client.println(buffer);
        }
    }

    if(url.endsWith(".html")){
        client.println(F("Content-Type: text/html"));
    } else if(url.endsWith(".json")){
        client.println(F("Content-Type: application/json"));
    }
    client.println(F("Connection: close"));
    client.println();

    if(url.equals("/")) {
        printIndexPage(client);
    } else if(url.equals("/sensors.json")){
        printSensorsJson(client);
    } else if(url.equals("/battery.json")){
        printCellVoltages(client);
        printBmsFaults(client);
        printBmsStates(client);
    } else if(url.equals("/switches.json")){
        client.println(R"===({ "switches": [)===");
        char buffer[64] = {0};
        sprintf(buffer,R"===({"name": "Imaging Computer 1", "state": %s},)===", ports[0] ? "true" : "false");
        client.println(buffer);
        sprintf(buffer,R"===({"name": "Imaging Computer 2", "state": %s},)===", ports[1] ? "true" : "false");
        client.println(buffer);
        sprintf(buffer,R"===({"name": "Port 3", "state": %s},)===", ports[2] ? "true" : "false");
        client.println(buffer);
        sprintf(buffer,R"===({"name": "Port 4", "state": %s})===", ports[3] ? "true" : "false");
        client.println(buffer);
        client.println(R"===(]})===");
    }
}

void printIndexPage(EthernetClient &client) {
    for(auto line : pageTop){
        client.println(line);
    }
    char buffer[265] = {0};
    for(int i =0; i< NUM_PORTS; i++){
        client.println(R"===(<tr>)===");
        sprintf(buffer, R"===(<td class="align-middle" id="n%d"></td>)===", i);
        client.println(buffer);
        sprintf(buffer, R"===(<td class="align-middle"><div id="s%d"></div></td>)===", i);
        client.println(buffer);
        sprintf(buffer, R"===(<td class="align-middle"><form method="post"><input name="power%d" type="hidden" value="1" id="i%d"><button type="submit" id="b%d"></button></form></td>)===", i, i ,i);
        client.println(buffer);
        sprintf(buffer, R"===(<td class="align-middle"><form method="post"><input name="power%d" type="hidden" value="2"><button type="submit" class="btn btn-dark btn-block">Cycle</button></form></td>)===", i);
        client.println(buffer);
        sprintf(buffer, R"===(</tr>)===", i);
        client.println(buffer);
    }
    for(auto line : pageBottom){
        client.println(line);
    }
}

void printSensorsJson(EthernetClient &client) {
    client.println(R"===({ "values":[)===");
    for(int i = 0; i < numSensorRecords; i++){
        char buffer[128] = {0};
        tmElements_t elements;
        breakTime(sensorData[i].readoutTime,elements);
        sprintf(buffer, R"===({"time":"%02d-%02d %02d:%02d", "pressure":%s, "temp":%s, "humidity":%s})===", elements.Month, elements.Day, elements.Hour, elements.Minute,
                String(sensorData[i].pressure).c_str(), String(sensorData[i].temperature).c_str(), String(sensorData[i].humidity).c_str());
        client.print(buffer);
        if(i != numSensorRecords - 1) {
            client.println(",");
        } else {
            client.println();
        }
    }
    client.println("]}");
}

void printBmsStates(EthernetClient &client) {
    char buffer[64] = {0};
    sprintf(buffer, R"===("charge": "%sA",)===", String(bms.current < 0 ? 0 : bms.current).c_str());
    client.println(buffer);
    sprintf(buffer, R"===("discharge": "%sA",)===", String(bms.current < 0 ? -bms.current : 0).c_str());
    client.println(buffer);
    sprintf(buffer, R"===("totalVoltage": "%sV",)===", String(bms.totalVoltage).c_str());
    client.println(buffer);
    sprintf(buffer, R"===("remainingSOC": %d,)===", bms.stateOfCharge);
    client.println(buffer);
    sprintf(buffer, R"===("minVoltage": "%sV",)===", String(bms.minVoltage24).c_str());
    client.println(buffer);
    sprintf(buffer, R"===("maxVoltage": "%sV",)===", String(bms.maxVoltage24).c_str());
    client.println(buffer);
    sprintf(buffer, R"===("maxCharge": "%sA",)===", String(bms.maxCharge24).c_str());
    client.println(buffer);
    sprintf(buffer, R"===("maxDischarge": "%sA",)===", String(bms.maxDischarge24).c_str());
    client.println(buffer);
    sprintf(buffer, R"===("maxPower": "%sW",)===", String(bms.balanceCapacity).c_str());
    client.println(buffer);
    sprintf(buffer, R"===("temp1": "%sC",)===", String(bms.temperatures[0]).c_str());
    client.println(buffer);
    sprintf(buffer, R"===("temp2": "%sC")===", String(bms.temperatures[1]).c_str());
    client.println(buffer);
    client.println("}");
}

void printCellVoltages(EthernetClient &client) {
    client.println(R"===({ "cellVoltages":[)===");
    for(int i = 0; i < NUM_CELLS; i++){
        char buffer[64] = {0};
        sprintf(buffer, R"===({"cell":"%d", "cellVoltage":%s, "balancing": %s})===", i, String(bms.cellVoltages[i]).c_str(), bms.isBalancing(i) ? "true" : "false");
        client.print(buffer);
        if(i != NUM_CELLS - 1) {
            client.println(",");
        } else {
            client.println();
        }
    }
    client.println(R"===(],)===");
}

void printBmsFaults(EthernetClient &client) {
    client.println(R"===("faults": [)===");
    char buffer[64] = {0};
    sprintf(buffer,R"===({"fault": "Single Cell Over-Voltage", "count": %d},)===", bms.faultCounts.singleCellOvervoltageProtection);
    client.println(buffer);
    sprintf(buffer,R"===({"fault": "Single Cell Under-Voltage", "count": %d},)===", bms.faultCounts.singleCellUndervoltageProtection);
    client.println(buffer);
    sprintf(buffer,R"===({"fault": "Whole Pack Over-Voltage", "count": %d},)===", bms.faultCounts.wholePackOvervoltageProtection);
    client.println(buffer);
    sprintf(buffer,R"===({"fault": "Whole Pack Under-Voltage", "count": %d},)===", bms.faultCounts.wholePackUndervoltageProtection);
    client.println(buffer);
    sprintf(buffer,R"===({"fault": "Charging Over Temperature", "count": %d},)===", bms.faultCounts.chargingOverTemperatureProtection);
    client.println(buffer);
    sprintf(buffer,R"===({"fault": "Charging Low Temperature", "count": %d},)===", bms.faultCounts.chargingLowTemperatureProtection);
    client.println(buffer);
    sprintf(buffer,R"===({"fault": "Discharge Over Temperature", "count": %d},)===", bms.faultCounts.dischargeOverTemperatureProtection);
    client.println(buffer);
    sprintf(buffer,R"===({"fault": "Discharge Low Temperature", "count": %d},)===", bms.faultCounts.dischargeLowTemperatureProtection);
    client.println(buffer);
    sprintf(buffer,R"===({"fault": "Charging Over-Current", "count": %d},)===", bms.faultCounts.chargingOvercurrentProtection);
    client.println(buffer);
    sprintf(buffer,R"===({"fault": "Discharge Over-Current", "count": %d},)===", bms.faultCounts.dischargeOvercurrentProtection);
    client.println(buffer);
    sprintf(buffer,R"===({"fault": "Short Circuit", "count": %d},)===", bms.faultCounts.shortCircuitProtection);
    client.println(buffer);
    sprintf(buffer,R"===({"fault": "Front End Detection Ic Error", "count": %d},)===", bms.faultCounts.frontEndDetectionIcError);
    client.println(buffer);
    sprintf(buffer,R"===({"fault": "Software Lock Mos", "count": %d})===", bms.faultCounts.softwareLockMos);
    client.println(buffer);
    client.println(R"===(],)===");
}

// send an NTP request to the time server at the given address
void sendNtpPacket(const char * address) {
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, ntpPacketSize);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    Udp.beginPacket(address, 123); // NTP requests are to port 123
    Udp.write(packetBuffer, ntpPacketSize);
    Udp.endPacket();
}

time_t getNtpTime() {
    while (Udp.parsePacket() > 0) ; // discard any previously received packets
    sendNtpPacket(ntpServer);
    uint32_t beginWait = millis();
    uint32_t now = beginWait;
    while (beginWait > now - 2000) {
        int size = Udp.parsePacket();
        if (size >= ntpPacketSize) {
            Udp.read(packetBuffer, ntpPacketSize);  // read packet into the buffer
            unsigned long secsSince1900;
            // convert four bytes starting at location 40 to a long integer
            secsSince1900 =  (unsigned long)packetBuffer[40] << (uint8_t) 24;
            secsSince1900 |= (unsigned long)packetBuffer[41] << (uint8_t) 16;
            secsSince1900 |= (unsigned long)packetBuffer[42] << (uint8_t) 8;
            secsSince1900 |= (unsigned long)packetBuffer[43];
            return secsSince1900 - NTP_OFFSET + UNIX_OFFSET + timeZone * SECS_PER_HOUR;
        }
        now = millis();
    }
    return 0; // return 0 if unable to get the time
}

