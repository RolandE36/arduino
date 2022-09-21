//                    Reset   |1 _ 8|   5V      - 3.3V
// Moisture Data  -   A3 D3   |2   7|   D2 A1   
// Moisture Power -   A2 D4   |3   6|   D1 PWM  - Transmitter Power
//           GND  -     GND   |4   5|   D0 PWM  - Transmitter Data

#include <GyverPower.h>

#define TRANSMITTER_DATA  0
#define TRANSMITTER_POWER 1

#define MOISTURE_DATA  A3
#define MOISTURE_POWER 4

#define DelayShort 188
#define DelayLong 564
#define DelaySync 5640
#define num_msgs 5      // number of messages to transmit

#define AIR_VAL   575   // you need to replace this value with Value_1 - 620
#define WATER_VAL 215   // you need to replace this value with Value_2 - 310
int soilMoistureValue   = 0;
int soilMoisturePercent = 0;

byte temp;

void setup () {
  power.hardwareDisable(PWR_ALL);
  power.hardwareEnable(PWR_ADC);  
  power.setSleepMode(POWERDOWN_SLEEP);

  pinMode(MOISTURE_POWER, OUTPUT);
  pinMode(TRANSMITTER_DATA,  OUTPUT);
  pinMode(TRANSMITTER_POWER, OUTPUT);
}

void loop () {
  digitalWrite(MOISTURE_POWER, HIGH);
  digitalWrite(TRANSMITTER_POWER, HIGH);
  digitalWrite(TRANSMITTER_DATA, 0);
  
  readSoilMoisture();
  digitalWrite(MOISTURE_POWER, LOW);
  
  transmitData(byte(13), byte(0), byte(soilMoisturePercent), byte(0));
  digitalWrite(TRANSMITTER_POWER, LOW);

  //power.sleepDelay(60000);   // 1m
  //power.sleepDelay(300000);  // 5m
  //power.sleepDelay(900000);  // 15m
  power.sleepDelay(3600000); // 60m
  //power.sleepDelay(7200000);   // 2h
}

void transmitData(byte mcode, byte sensorId, byte val1, byte val2) {
  temp = num_msgs;

  while (temp > 0) {
    writeRF(mcode);     // Check code
    writeRF(sensorId);  // Sensor
    writeRF(val1);      // Value 1
    writeRF(val2);      // Value 2
    stopBit();
    delayMicroseconds(DelaySync);
    temp--;
  }
}

void stopBit () {
  digitalWrite(TRANSMITTER_DATA, 1);
  delayMicroseconds(DelayShort);
  digitalWrite(TRANSMITTER_DATA, 0);
  delayMicroseconds(DelayLong);
}

void writeRF (byte Data_Byte) {
  byte i = 8; // send MSB first

  while (i > 0) {
    i--;
    digitalWrite(TRANSMITTER_DATA, 1);

    if (bitRead(Data_Byte, i) == 0) {
      delayMicroseconds(DelayShort);
      digitalWrite(TRANSMITTER_DATA, 0);
      delayMicroseconds(DelayLong);
    } else {
      delayMicroseconds(DelayLong);
      digitalWrite(TRANSMITTER_DATA, 0);
      delayMicroseconds(DelayShort);
    }
  }
}

void readSoilMoisture() {
    delay(200);
    soilMoistureValue = analogRead(MOISTURE_DATA);
    
    soilMoisturePercent = map(soilMoistureValue, AIR_VAL, WATER_VAL, 0, 100);
    if (soilMoisturePercent >= 100) soilMoisturePercent = 100;
    if (soilMoisturePercent <= 0)   soilMoisturePercent = 0;
}
