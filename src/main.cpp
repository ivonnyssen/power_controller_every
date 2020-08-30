/*
 Power Controller on a web page
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Relay module is attached to digital pins 5, 6, 7, 8
 */

#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>
#include "page.h"
#include <TimeLib.h>

#define GET 0
#define POST 1
#define UNSUPPORTED 2

#define OFF 0
#define ON 1
#define CYCLE 2

#define DHTPIN 2
#define DHTTYPE DHT11

#define DEBUG true

#if DEBUG
 #define DEBUG_SERIAL_PRINTLN(x) if(Serial) Serial.println(x)
#else
 #define DEBUG_SERIAL_PRINTLN(x)
#endif

typedef struct Requests{
    int type;
    String url;
    long powerPort;
    long command;
} Request;

typedef struct Sensors{
    time_t readoutTime;
    float pressure;
    float temperature;
    float humidity;
} SensorData;

Request parseRequest(EthernetClient client);

void readAndLogRequestLines(EthernetClient client);

void printWebPage(EthernetClient client, const String &url, int type);

void sendNTPPacket(const char * address);

void handleHttpRequest(EthernetClient &client);

time_t getNtpTime();

void measureAndLogSensors(time_t &now);

String zeroPad(int);

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
const int NTP_PACKET_SIZE = 48;
uint8_t packetBuffer[NTP_PACKET_SIZE];
const int timeZone = -8; //PST
unsigned int localPort = 8888;
EthernetUDP Udp;
time_t lastLogTime;

// Queue for barometric pressure values
const int NUM_SENSOR_RECORDS = 24 * 4;
SensorData sensorData[NUM_SENSOR_RECORDS];

//variables for states
bool ports[4] {false,false,false,false};
#define BASE_PORT_PIN 3

//temp sensor
DHT dht(DHTPIN, DHTTYPE);

void setup() {
    // Open serial communications and wait for port to open:
    Serial.begin(9600);

    // start the Ethernet connection and the server:
    EthernetClass::begin(mac);
    server.begin();
    DEBUG_SERIAL_PRINTLN(EthernetClass::localIP());

    //UDP for NTP
    Udp.begin(localPort);
    delay(2000);
    setSyncProvider(getNtpTime);
    setSyncInterval(SECS_PER_HOUR);
    if(timeStatus() != timeSet) DEBUG_SERIAL_PRINTLN("Unable to sync with NTP");
    DEBUG_SERIAL_PRINTLN(now());

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

    //temperature & humidity
    dht.begin();
}

void  loop() {

    // listen for incoming clients
    EthernetClient client = server.available();
    if (client) {
        handleHttpRequest(client);
    }

    time_t seconds = now();
    if(seconds % 10 == 0 && seconds != lastLogTime) {
        measureAndLogSensors(seconds);
        lastLogTime = seconds;
    }
}

void measureAndLogSensors(time_t &now) {
    for(int i = 0; i < NUM_SENSOR_RECORDS - 1; i++){
        sensorData[i] = sensorData[i + 1];
    }
    sensorData[NUM_SENSOR_RECORDS - 1] = {now, 980, dht.readTemperature(), dht.readHumidity()};
    String logLine = String((unsigned long) sensorData[NUM_SENSOR_RECORDS - 1].readoutTime);
    logLine.concat(',');
    logLine.concat(sensorData[NUM_SENSOR_RECORDS -1].pressure);
    logLine.concat(',');
    logLine.concat(sensorData[NUM_SENSOR_RECORDS -1].temperature);
    logLine.concat(',');
    logLine.concat(sensorData[NUM_SENSOR_RECORDS -1].humidity);
    logLine.concat('\n');
    DEBUG_SERIAL_PRINTLN(logLine.c_str());
}

void handleHttpRequest(EthernetClient &client) {
    DEBUG_SERIAL_PRINTLN(client.available());
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
    DEBUG_SERIAL_PRINTLN(s);
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
    DEBUG_SERIAL_PRINTLN(result.url);
    return result;
}

void readAndLogRequestLines(EthernetClient client) {
    while (client.available()) {
        String s = client.readStringUntil('\n');
        DEBUG_SERIAL_PRINTLN(s);
        if(s.equals(String('\r'))){
            break;
        }
    }
}

void printWebPage(EthernetClient client, const String &url, const int type) {
    //print header
    if (type == POST) {
        client.println("HTTP/1.1 303 See Other");
        String location = "Location: http://";
        location.concat((int)EthernetClass::localIP()[0]);
        location.concat('.');
        location.concat((int)EthernetClass::localIP()[1]);
        location.concat('.');
        location.concat((int)EthernetClass::localIP()[2]);
        location.concat('.');
        location.concat((int)EthernetClass::localIP()[3]);
        location.concat(url);
        client.println(location);
    } else {
        client.println("HTTP/1.1 200 OK");
    }

    if(url.endsWith(".htm")){
        client.println("Content-Type: text/html");
    } else if(url.endsWith(".jsn")){
        client.println("Content-Type: application/json");
    }
    client.println("Connection: close");
    client.println();

    if(url.equals("/")) {
        for(auto line : pageTop){
            client.write(line);
        }
        char buffer[256] = {0};

        for( int i = 0; i < NUM_PORTS;  i++){
            client.write(F(R"===(<tr>)==="));
            sprintf(buffer,F(R"===(<td class="align-middle">%s</td>)==="), items[i]);
            client.write(buffer);
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, F(R"===(<td class="align-middle"><div class="alert-sm alert-%s text-center">%s</div></td>)==="), ports[i] ? "success" : "danger", ports[i] ? "On" : "Off");
            client.write(buffer);
            sprintf(buffer, F(R"===(<td class="align-middle"><form method="post"><input name="power%d" type="hidden" value="%s"><button type="submit" class="btn btn-block btn-%s">%s</button></form></td>)==="), i, ports[i] ? "0" : "1", ports[i] ? "danger" : "success", ports[i] ? "Off" : "On");
            client.write(buffer);
            sprintf(buffer, F(R"===(<td class="align-middle"><form method="post"><input name="power%d" type="hidden" value="2"><button type="submit" class="btn btn-dark btn-block">Cycle</button></form></td>)==="), i);
            client.write(buffer);
            client.write(F(R"===(</tr>)==="));
        }

        for(auto line : pageBottom){
            client.write(line);
        }
    } else if(url.equals("/sensors.jsn")){
        client.println("{ \"values\":[");
        for(int i = 0; i < NUM_SENSOR_RECORDS; i++){
            String line = String(R"({"time":")");
            tmElements_t elements;
            breakTime(sensorData[i].readoutTime,elements);
            line.concat(elements.Year + 1970);
            line.concat('-');
            line.concat(zeroPad(elements.Month));
            line.concat('-');
            line.concat(zeroPad(elements.Day));
            line.concat(' ');
            line.concat(zeroPad(elements.Hour));
            line.concat(':');
            line.concat(zeroPad(elements.Minute));
            line.concat(' ');
            line.concat(R"(", "pressure":)");
            line.concat(sensorData[i].pressure);
            line.concat(R"(, "temp":)");
            line.concat(sensorData[i].temperature);
            line.concat(R"(, "humidity":)");
            line.concat(sensorData[i].humidity);
            line.concat('}');
            if(i != NUM_SENSOR_RECORDS - 1) {
                line.concat(",\n");
            } else {
                line.concat('\n');
            }
            client.write(line.c_str());
        }
        client.println("]}");
    }
}

String zeroPad(int value){
    String result;
    if(value < 10) result.concat("0");
    result.concat(value);
    return result;
}

// send an NTP request to the time server at the given address
void sendNTPPacket(const char * address) {
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
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
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
}

time_t getNtpTime() {
    while (Udp.parsePacket() > 0) ; // discard any previously received packets
    sendNTPPacket(ntpServer);
    uint32_t beginWait = millis();
    uint32_t now = beginWait;
    while (beginWait > now - 2000) {
        int size = Udp.parsePacket();
        if (size >= NTP_PACKET_SIZE) {
            Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
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

