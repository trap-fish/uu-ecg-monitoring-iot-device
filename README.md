# ECG Monitoring Device
Coursework group project for creating an IoT Stack for an ECG Monitor

## TIG Network Stack
Telegraf - Influx - Grafana was based on the [repo by huntabyte](https://github.com/huntabyte/tig-stack), this was adatped to utilize MQTT

## ECG Sensor
This is the code deployed on an Arduino MKR1000 and required secrets file (only an example secrets.h is uploaded). An external breakout circuit was built, the output of which is read through a 12-bit ADC (MCP3208P) via SPI
