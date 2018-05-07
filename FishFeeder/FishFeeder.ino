#include <Wire.h>
#include <Time.h>
#include <DS1307RTC.h>
#include <LowPower.h>

tmElements_t tm; // Time

void setup() {
  pinMode(13, OUTPUT);
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  digitalWrite(A0, HIGH);
  digitalWrite(A1, HIGH);
  
  Serial.begin(9600);
  while (!Serial) ; // wait for serial
  delay(200);

  if (RTC.read(tm)) {
    feedNow();
  } else {
    rtcError();
  }
}

void loop() {
  if (RTC.read(tm)) {
    feed();
  } else {
    rtcError();
  }

  // http://www.rocketscream.com/blog/2011/07/04/lightweight-low-power-arduino-library/
  // https://github.com/rocketscream/Low-Power
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}

// Feed fish if it's time to do
void feed() {
  if ((tm.Hour == 9 || tm.Hour == 20) && tm.Minute == 1) {
  //if (tm.Minute % 10 == 0) {
    feedNow();
  }
}

// Feed fish right now
void feedNow() {
    delay(1000);
    digitalWrite(A1, LOW);
    delay(600);
    digitalWrite(A1, HIGH);

    Serial.print("Feed, ");
    printTime();
    delay(62000);
}

// Print provided tome
void printTime() {
  Serial.print("Time = ");
  print2digits(tm.Hour);
  Serial.write(':');
  print2digits(tm.Minute);
  Serial.write(':');
  print2digits(tm.Second);
  Serial.print(", Date (D/M/Y) = ");
  Serial.print(tm.Day);
  Serial.write('/');
  Serial.print(tm.Month);
  Serial.write('/');
  Serial.print(tmYearToCalendar(tm.Year));
  Serial.println();
}

// Print time on two digits format
void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}

// Analyze RTC error reason
void rtcError() {
  if (RTC.chipPresent()) {
    Serial.println("The DS1307 is stopped. Please run the SetTime");
    Serial.println();

    // The DS1307 is stopped. Try to reinitialize with default time.
    if (RTC.chipPresent()) {
      setDefaultTime(); 
      rtcError();
    }
  } else {
    Serial.println("DS1307 read error! Please check the circuitry.");
    Serial.println();
  }

  error();
  delay(9000);
}

// Blink SOS message
void error() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(13, HIGH);
    delay(1000);
    
    digitalWrite(13, LOW); 
    delay(400);
    digitalWrite(13, HIGH);
    delay(400);

    digitalWrite(13, LOW); 
    delay(400);
    digitalWrite(13, HIGH);
    delay(400);

    digitalWrite(13, LOW); 
    delay(400);
    digitalWrite(13, HIGH);
    delay(1000);

    digitalWrite(13, LOW); 
  }
}

// Set default time 05/05/2018 19:50
void setDefaultTime() {
  tm.Hour = 19;
  tm.Minute = 50;
  tm.Second = 0;
  tm.Day = 5;
  tm.Month = 5;
  tm.Year = CalendarYrToTm(2017);
}

