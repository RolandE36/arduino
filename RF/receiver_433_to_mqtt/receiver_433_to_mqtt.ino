#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <RCSwitch.h>

//
// TRANSCEIVER -  - RECEIVER    
//             |  |
//             |  |
//        |             |
//        |   ESP-01    |
//        |             |
//        |  G 2 0 *    |
//        |  * * * 3.3v |
//        |    |___|    |

// ESP-01
#define RECEIVER_DATA 0
#define TRANSMITTER_DATA 2

#define MESSAGE_LENGHT 4
#define DELAY_SHORT 188
#define DELAY_LONG 564
#define DELAY_SYNC 5640
#define NUM_MSGS 5

const char *WIFI_SSID = "";
const char *WIFI_PASSWORD = "";

const char *MQTT_USER = "";
const char *MQTT_PASSWORD = "";
const char *MQTT_HOST = ".duckdns.org";
const int   MQTT_PORT = 1883;
const char *MQTT_CLIENT_ID = "rf2mqtt";

const char *TOPIC_STATUS = "rf2mqtt/online";
const char *TOPIC_LOG = "rf2mqtt/log";
const char *TOPIC_LISTEN = "rf2mqtt/transmitter/+/value";
const byte  TRANSMIT_CODE = 90;
const byte  RECEIVE_CODE  = 13;

bool isNewMessage433toMqtt = false;
bool isNewMessageMqttTo433 = false;
bool nextIsNewMessage433toMqtt = false;

int sensorId = 0;
int sensorValue = 0;
byte temp;

byte byte1;
byte byte2;
byte byte3;

RCSwitch mySwitch = RCSwitch();
WiFiClient client;
PubSubClient mqttClient(client);
 
void setup() {  
    //Serial.begin(9600);
    //Serial.println("Starting...");
    
    setupWifi();
    
    delay(5000);
    reconnectMqtt();

    delay(2000);
    pinMode(TRANSMITTER_DATA,  OUTPUT);
    mySwitch.enableReceive(RECEIVER_DATA);
    mqttClient.publish(TOPIC_STATUS, "on");

    delay(1000);
    //Serial.println("Started");
}
 
void loop() {
    reconnectToWIFI();    
    reconnectMqtt();
    
    read433();
    sendMqtt();
    
    mqttClient.loop();
    send433();
}

void setupWifi() {
    delay(10);
    // We start by connecting to a WiFi network
    //Serial.println();
    //Serial.print("Connecting to ");
    //Serial.println(WIFI_SSID);
  
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        //Serial.print(".");
    }
 
    //Serial.println("");
    //Serial.println("WiFi connected");
    //Serial.println("IP address: ");
    //Serial.println(WiFi.localIP());
}

void reconnectToWIFI() {
    if (WiFi.status() != WL_CONNECTED) {
        //Serial.println("Reconnecting to WIFI network");
        WiFi.disconnect();
        WiFi.reconnect();
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            //Serial.print(".");
        }
    }
}

void read433() {
    if (!mySwitch.available()) return;
    
    unsigned long receivedValue;
  
    //Serial.print("Received from 433: ");
    if (mySwitch.getReceivedBitlength() == 8 * MESSAGE_LENGHT) {
        receivedValue = mySwitch.getReceivedValue();
    
        byte message[MESSAGE_LENGHT];
    
        for (int i = 0; i < MESSAGE_LENGHT; i++) {
            message[i] = receivedValue & 0xFF;
            receivedValue = receivedValue >> 8;
            //Serial.print(message[i]);
            //Serial.print("-");
        }

        // message[3] - Check code = 13
        // message[2] - Sensor     = 0-255
        // message[1] - Value 1    = 0-255
        // message[0] - Check code = 0
  
        // Verification for 'one message' transmitters
        if (message[3] == RECEIVE_CODE && message[0] == 0) isNewMessage433toMqtt = true;

        // Verification for 'multiple message' transmitters
        if (nextIsNewMessage433toMqtt) {
            isNewMessage433toMqtt = true;
            nextIsNewMessage433toMqtt = false;
        }
        if (message[3] == RECEIVE_CODE && 
            message[2] == RECEIVE_CODE &&
            message[1] == RECEIVE_CODE &&
            message[0] == RECEIVE_CODE) nextIsNewMessage433toMqtt = true;        
        
        sensorId = message[2];
        sensorValue = message[1];
    }

    // mySwitch.getReceivedProtocol()
    // mySwitch.getReceivedBitlength()
    // mySwitch.getReceivedValue()

    // Log reveived values
    char buf[20];
    receivedValue = mySwitch.getReceivedValue();
    String(receivedValue).toCharArray(buf, 20);
    mqttClient.publish(TOPIC_LOG, buf);
    delay(1000);
    
    mySwitch.resetAvailable();
}

void sendMqtt() {
    if(!isNewMessage433toMqtt) return;
    
    isNewMessage433toMqtt = false;  
    char charBuf[50];

    //Serial.println("Saved to MQTT:");
    
    String mqttMessageOn = "rf2mqtt/receiver/" + String(sensorId) + "/online";
    mqttMessageOn.toCharArray(charBuf, 50);
    mqttClient.publish(charBuf, "on");
    //Serial.println(" - " + mqttMessageOn + " " + "on");
  
    String mqttMessageVal = "rf2mqtt/receiver/" + String(sensorId) + "/value";
    mqttMessageVal.toCharArray(charBuf, 50);
    mqttClient.publish(charBuf, String(sensorValue).c_str(), true);
    //Serial.println(" - " + mqttMessageVal + " " + sensorValue);
    
    //int value = random(1, 101);
    //mqttClient.publish(TOPIC_SEND, String(value).c_str());  
}

void reconnectMqtt() {
    if (mqttClient.connected()) return;
    //digitalWrite(LED_BUILTIN, HIGH);
  
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
  
    // Loop until we're reconnected
    while (!mqttClient.connected()) {
        //Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
            //Serial.println("Connected to MQTT broker");
            // Subscribe
            mqttClient.subscribe(TOPIC_LISTEN);
            //digitalWrite(LED_BUILTIN, LOW);
        } else {
            //Serial.print("failed, rc=");
            //Serial.print(mqttClient.state());
            //Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void mqttCallback(char* topic, byte* message, unsigned int length) {
    String messageTemp;
    String topicString;
    
    for (int i = 0; i < length; i++) {
        messageTemp += (char)message[i];
    }
    for (int i = 0; i < strlen(topic); i++) {
        topicString += (char)topic[i];
    }

    String nreceiver;
    int nstart = 0;
    int nend = 0;
    
    nstart    = topicString.indexOf('/');              // first  / - rf2mqtt/
    nstart    = topicString.indexOf('/', nstart+1);    // second / - rf2mqtt/transmitter/
    nend      = topicString.indexOf('/', nstart+1);    // third  / - rf2mqtt/transmitter/+/
    nreceiver = topicString.substring(nstart+1, nend); // number

    //Serial.println("Received from MQTT: ");
    //Serial.println(" - Topic: " + String(topic));
    //Serial.println(" - Message: " + messageTemp);

    isNewMessageMqttTo433 = true;
    byte1 = byte(nreceiver.toInt());
    byte2 = byte(messageTemp.toInt());
    byte3 = byte(0);
}

void send433() {
    if (!isNewMessageMqttTo433) return;
    isNewMessageMqttTo433 = false;
    temp = NUM_MSGS;
  
    while (temp > 0) {
        writeRF(TRANSMIT_CODE);// Check code
        writeRF(byte1);       // Sensor
        writeRF(byte2);       // Value 1
        writeRF(byte3);       // Value 2
        stopBit();
        delayMicroseconds(DELAY_SYNC);
        temp--;
    }

    mqttClient.publish(TOPIC_LOG, "send433");
}

void stopBit () {
    digitalWrite(TRANSMITTER_DATA, 1);
    delayMicroseconds(DELAY_SHORT);
    digitalWrite(TRANSMITTER_DATA, 0);
    delayMicroseconds(DELAY_LONG);
}

void writeRF (byte Data_Byte) {
    byte i = 8; // send MSB first
  
    while (i > 0) {
        i--;
        digitalWrite(TRANSMITTER_DATA, 1);
    
        if (bitRead(Data_Byte, i) == 0) {
            delayMicroseconds(DELAY_SHORT);
            digitalWrite(TRANSMITTER_DATA, 0);
            delayMicroseconds(DELAY_LONG);
        } else {
            delayMicroseconds(DELAY_LONG);
            digitalWrite(TRANSMITTER_DATA, 0);
            delayMicroseconds(DELAY_SHORT);
        }
    }
}
