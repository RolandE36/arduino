/*
  Simple example for receiving
  
  https://github.com/sui77/rc-switch/
*/

#include <Servo.h>
#include <RCSwitch.h>


#define MESSAGE_LENGHT 4

RCSwitch mySwitch = RCSwitch();
Servo myservo;
int newAngle = 0;
int currentAndle = 180;

// rf2mqtt/transmitter/1/value

void setup() {
  Serial.begin(9600);
  mySwitch.enableReceive(0);  // Receiver on interrupt 0 => that is pin #2
  delay(100);
  
  Serial.println("Working");
}

void loop() {
  /*if (Serial.available() > 0) {
    currentAndle = newAngle;
    newAngle = Serial.parseInt();
    Serial.println("parseInt");  
  }*/ 
  
  if (mySwitch.available()) {
    Serial.println("Received");

    unsigned long  receivedValue = mySwitch.getReceivedValue();
    int bitlenght = mySwitch.getReceivedBitlength();
    mySwitch.resetAvailable();
    
    if (bitlenght != 8 * MESSAGE_LENGHT) return;

    byte message[MESSAGE_LENGHT];
    
    for (int i = 0; i < MESSAGE_LENGHT; i++) {
        message[i] = receivedValue & 0xFF;
        receivedValue = receivedValue >> 8;
    }

    if (message[0] != 0)  return; // Code
    if (message[2] != 1)  return; // Sensor
    if (message[3] != 90) return; // Code

    currentAndle = newAngle;
    newAngle = message[1];

    Serial.println(message[0]);
    Serial.println(message[1]);
    Serial.println(message[2]);
    Serial.println(message[3]);
  }

  if (newAngle != currentAndle) {    
    myservo.attach(3);

    if (newAngle < currentAndle) {
        for (int i = currentAndle; i >= newAngle; i-=1) {
            myservo.write(i);
            delay(10);
        }
    } else {
        for (int i = currentAndle; i <= newAngle; i+=1) {
            myservo.write(i);
            delay(10);
        }
    }

    currentAndle = newAngle;
    myservo.detach();
  }
}
