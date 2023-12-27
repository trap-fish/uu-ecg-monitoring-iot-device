#include <MQTT.h>
#include <SPI.h>
#include <WiFi101.h>
#include <Mcp320x.h>

#include "secrets.h"

// setup network connection strings
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

// setup MQTT credentials
char mqttUsername[] = SECRET_MQTT_USERNAME;
char mqttPassword[] = SECRET_MQTT_PASSWORD;

// setup the ADC chip pin, Vcc and clock values
const uint8_t csPin = 3;
const uint16_t vref = 3300;
const uint32_t clock = 1600000;

// init Mcp320x object, using MCP3208 namespace
MCP3208 adc(vref,csPin);

// setup the publishing intervals
unsigned long lastPublished = 0;
unsigned const long publishInterval = 1 * 50L;

// variables for the bpm calculation
uint16_t beatsPerMin = 0;
unsigned long int  previousQRS = 0;
const uint16_t upThreshold = 850;
const uint16_t lowThreshold = 700;
bool isQRS = false;

// init the Wifi client and MQTT client libraries
WiFiClient wifiClient;
MQTTClient mqttClient;


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
  mqttClient.begin("xxx.xxx.xx.xx", wifiClient); // update the IP address for mqtt broker
  delay(1000);
}

void loop() {
  mqttClient.loop();

  // connect if not connected and print status
  if (!mqttClient.connected()) {
    connect();
    Serial.print("Connected? ");
    Serial.println(mqttClient.connected());
  }

  // read from ADC MCP3208 via SPI
  uint16_t raw = adc.read(MCP3208::Channel::SINGLE_3);  // read data from CH3
  uint16_t val = adc.toAnalog(raw); // get analog conversion of data

  // converts data into an influxDB line protocol format
  String payload1 = getPayload("ecg_001", val);

  // sends data after a delay interval, uncomment to publish every publishInterval ms
  //while(millis() - lastPublished < publishInterval){
  //}

  mqttClient.publish("sensors/ecg_device1", payload1);

  // reset lastPublished if using the delay interval
  //lastPublished = millis();

  calculateBPM(val); // calculate the BPM
}

// conneccts to WiFi and MQTT Broker
void connect() {
  // check wifi status and connect if not connected
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    //continue to connect until connection is successful
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass);  // Connect to Wifi network
      Serial.print(".");
      delay(5000);     //wait before trying again in case not reconnected
    } 
    Serial.println("\nConnected.");
  }

  // reconnects to MQTT (already connected in setup)
  while (!mqttClient.connect("arduino", mqttUsername, mqttPassword)){
    Serial.print("mqtt disconnection...");
    Serial.print(".");
    delay(1000);
  }

  Serial.println("Connected to MQTT Broker");
  mqttClient.subscribe("test/topic");
}

// format into an Influx line protocol
String getPayload(String measurement, int voltage) {
    String fluxLine = measurement + ",device=device1" + " ecg=" + String(voltage);

    return fluxLine;
}

// calculates the BPM
void calculateBPM(uint16_t reading) {
  // set QRS flag to true if over threshold and not already true
  if (reading > upThreshold && isQRS == false) {
    isQRS = true;
    unsigned long int currentQRS = millis(); // update the new QRS time
    unsigned long int rInterval = currentQRS - previousQRS;
    previousQRS = currentQRS; // update the previousQRS for the next call
    uint16_t bpm = 60000 / rInterval;
    Serial.print(bpm);
  } 
  // set flag to false if lower threshold is passed
  else if (reading < lowThreshold && isQRS == true) {
    isQRS = false;
  }
}