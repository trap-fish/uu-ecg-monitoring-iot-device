#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>
#include <WiFi101.h>

#include <SPI.h>
#include <Mcp320x.h>

#include "secrets.h"

// setup network connection strings
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

// setup MQTT credentials
const char broker[] = SECRET_BROKER;
const char* certificate = SECRET_CERTIFICATE;

// setup the ADC chip pin, Vcc and clock values
const uint8_t csPin = 3;
const uint16_t vref = 3300;
const uint32_t clock = 1600000;

// init Mcp320x object, using MCP3208 namespace
MCP3208 adc(vref, csPin);


// variables for the bpm calculation
uint16_t beatsPerMin = 0;
unsigned long int previousQRS = 0;
bool isQRS = false;

// init the Wifi client, SSL and MQTT client libraries
WiFiClient wifiClient;
BearSSLClient sslClient(wifiClient);
MqttClient mqttClient(sslClient);


// for json string, increases each iteration
uint32_t measurement_id = 0;


void setup() {
  // init serial
  Serial.begin(115200);

  // configure the SPI pin mode and initial state (high)
  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH);

  //config SPI interface (Clock, bit order, SPI mode), most sig bit and Mode 0 (idles low)
  SPISettings settings(clock, MSBFIRST, SPI_MODE0);
  SPI.begin();                     //Begins SPI
  SPI.beginTransaction(settings);  //Sets transaction to config defined above

  // network and mqtt
  WiFi.begin(ssid, pass);

  mqttClient.connect(SECRET_BROKER, 8883);  // update the IP address for mqtt broker
  delay(2000);

  // time needed for SSL certificate validation
  ArduinoBearSSL.onGetTime(sessionStartTime);
  // Private key slot and public key certificate
  sslClient.setEccSlot(0, certificate);
}


void loop() {
  
  // connect to wifi if not connected
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // connect if not connected and print status
  if (!mqttClient.connected()) {
    connectMQTT();
  }

  // poll for new MQTT messages and send keep alives
  mqttClient.poll();

  // read from ADC MCP3208 via SPI
  uint16_t raw = adc.read(MCP3208::Channel::SINGLE_3);  // read data from CH3
  uint16_t measurement = adc.toAnalog(raw);                     // get analog conversion of data
  
  measurement_id++;

  // define the thresholds for R wave detection
  const uint16_t upThreshold = 850;
  const uint16_t lowThreshold = 700;
  // set QRS flag to true if over threshold and not already true
  if (val > upThreshold && isQRS == false) {
    isQRS = true;
    calculateBPM();
  }  // set flag to false if lower threshold is passed
  else if (val < lowThreshold && isQRS == true) {
    isQRS = false;
  }

  publishJson(measurement_id, measurement, beatsPerMin);
}


// conneccts to WiFi and MQTT Broker
void connectWiFi() {
  // check wifi status and connect if not connected
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(SECRET_SSID);
  //continue to connect until connection is successful
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    WiFi.begin(ssid, pass);  // Connect to Wifi network
    Serial.print(".");
    delay(5000);  //wait before trying again in case not reconnected
  }
  Serial.println("\nConnected.");
}

void connectMQTT() {
  // reconnects to MQTT (already connected in setup)
  while (!mqttClient.connect(SECRET_BROKER, 8883)) {
    Serial.print("mqtt disconnection...");
    Serial.print(".");
    delay(1000);
  }
  Serial.println(mqttClient.connected());
  Serial.print(" - Connected to MQTT Broker");
  mqttClient.subscribe("test/topic");
}

unsigned long sessionStartTime() {
  // get the current time
  return WiFi.getTime();
}

void publishJson(uint16_t id, uint16_t ecg, uint16_t bpm) {

  // define the mqtt topic to publish to, 
  // use QoS=1 at least once, DB will overwrite duplicates
  mqttClient.beginMessage("device/22/data", false, 1, false);

  // generate the JSON string
  String jsonString = "{";
  jsonString += "\"measurement_id\":" + String(id) + ",";
  jsonString += "\"measurement\":" + String(ecg) + ",";
  jsonString += "\"heart_rate\":" + String(bpm);
  jsonString += "}";

  //publish and close
  mqttClient.print(jsonString);
  mqttClient.endMessage();
}

// calculates the BPM
void calculateBPM() {
  unsigned long int currentQRS = millis();  // update the new QRS time
  unsigned long int rInterval = currentQRS - previousQRS;
  previousQRS = currentQRS;  // update the previousQRS for the next call
  beatsPerMin = 60000 / rInterval;
}