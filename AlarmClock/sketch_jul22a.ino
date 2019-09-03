// https://www.instructables.com/id/Sounds-Unit-for-Toys-Using-DFplayer-Mini-MP3-Playe/
// http://arduiner.blogspot.com/2016/06/nodemcu-esp8266-dfplayer-mp3-music-from.html
// https://github.com/DFRobot/DFPlayer-Mini-mp3/blob/master/DFPlayer_Mini_Mp3/examples/DFPlayer_sample/DFPlayer_sample.ino
// https://www.electronicwings.com/nodemcu/nodemcu-gpio-with-arduino-ide

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266HTTPClient.h>
#include <FirebaseArduino.h>  
#include <ArduinoJson.h> // V 5.X.X
#include <DFPlayer_Mini_Mp3.h>
#include <SoftwareSerial.h>

#define STASSID ""
#define STAPSK  ""

#define FIREBASE_HOST "" // the project name address from firebase id
#define FIREBASE_AUTH "" // the secret key generated from firebase

#define PIN_BUSY 14
#define PIN_LED 1
//SoftwareSerial mp3Serial(5, 4); // RX, TX - NodeMCU
SoftwareSerial mp3Serial(0, 2);   // RX, TX - ESP-01

const char* host = "esp8266-webupdate";
const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

String json = "{}";
const char* formatted;
String date = "";
IPAddress localIP;
String alarm = "";
int ch = 0; // Currrent
int cm = 0;
int volume = 5;
int ah = 0; // Alaram
int am = 0;
bool isMusic = false;

void getTimestamp() {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    HTTPClient http;  //Declare an object of class HTTPClient
    http.begin("http://api.timezonedb.com/v2.1/get-time-zone?key=XXXXXXXXXXXXXXXXXX&format=json&by=zone&zone=Europe/Kiev");  //Specify request destination
    int httpCode = http.GET(); //Send the request
    
    if (httpCode > 0) { //Check the returning code
      json = http.getString();   //Get the request response payload
    } 

    http.end();   //Close connection
  } else {
    Serial.println("WiFi connection error");
    return;
  }

  StaticJsonBuffer<1024> jsonBuffer;
  JsonObject& root = jsonBuffer.parse(json);
  if (root.success()) {
    formatted = root["formatted"];
  } else {
    Serial.println("Json parsing error");  
    return;
  }

  date = "";
  date.concat(formatted[0]); // 2
  date.concat(formatted[1]); // 0
  date.concat(formatted[2]); // 1
  date.concat(formatted[3]); // 9
  date.concat(formatted[4]); // -
  date.concat(formatted[5]); // 0
  date.concat(formatted[6]); // 7
  date.concat(formatted[7]); // -
  date.concat(formatted[8]); // 2
  date.concat(formatted[9]); // 3
  date.concat(formatted[10]); //  
  date.concat(formatted[11]); // 0
  date.concat(formatted[12]); // 1
  date.concat(formatted[13]); // :
  date.concat(formatted[14]); // 1
  date.concat(formatted[15]); // 5
  Serial.print("Time: ");
  Serial.println(date);

  String sh = "";
  sh.concat(formatted[11]);
  sh.concat(formatted[12]);

  String sm = "";
  sm.concat(formatted[14]);
  sm.concat(formatted[15]);
  
  ch = sh.toInt();
  cm = sm.toInt();
}

void saveFirebaseIp() {
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.setString("/ALARM_CLOCK/IP", localIP.toString());
  Firebase.setString("/ALARM_CLOCK/IP", localIP.toString());
}

void saveFirebase() {
  saveFirebaseIp();
  Firebase.setInt("/ALARM_CLOCK/HOUR", ah);
  Firebase.setInt("/ALARM_CLOCK/MINUTE", am);
  Firebase.setInt("/ALARM_CLOCK/VOLUME", volume);

  alarm = "";
  if (ah < 10) alarm.concat("0");
  alarm.concat(String(ah));
  alarm.concat(":");
  if (am < 10) alarm.concat("0");
  alarm.concat(String(am));

  Serial.println("saveFirebase");
  Serial.println("Alarm time: " + alarm);
  Serial.println("Volume: " + String(volume));
}

void getFirebase() {
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  ah = Firebase.getInt("/ALARM_CLOCK/HOUR");
  am = Firebase.getInt("/ALARM_CLOCK/MINUTE");
  volume = Firebase.getInt("/ALARM_CLOCK/VOLUME");

  alarm = "";
  if (ah < 10) alarm.concat("0");
  alarm.concat(String(ah));
  alarm.concat(":");
  if (am < 10) alarm.concat("0");
  alarm.concat(String(am));

  Serial.println("getFirebase");
  Serial.println("Alarm time: " + alarm);
  Serial.println("Volume: " + String(volume));
}

void indexHandler() {
  String message = "";

  if (httpServer.arg("nohtml") == "") {
    message += "<!DOCTYPE html><html>";
    message += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    message += "<link rel=\"icon\" href=\"data:,\">";
    message += "<title>Alarm Clock</title>";
    message += "</head>";
    message += "<body>";
  
    message += "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/materialize/1.0.0/css/materialize.min.css'>";
    message += "<link href='https://fonts.googleapis.com/icon?family=Material+Icons' rel='stylesheet'>";
    message += "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js'></script>";
    
    message += "<div class='row'>";
    message += "  <div class='col s12 offset-m2 m8'>";
  
    message += "<div class='row'><br/>";
    message += "  <a onclick='$.get(\"/play?nohtml=1\"); return false;' href='/play' class='waves-effect waves-light btn'><i class='material-icons left'>play_arrow</i>Play</a>";
    message += "  <a onclick='$.get(\"/random?nohtml=1\"); return false;' href='/random' class='waves-effect waves-light btn'><i class='material-icons left'>all_inclusive</i>Random</a>";
    message += "  <a onclick='$.get(\"/pause?nohtml=1\"); return false;' href='/pause' class='waves-effect waves-light btn'><i class='material-icons left'>pause</i>Pause</a>";
    message += "  <br/>";
    message += "  <a onclick='$.get(\"/previous?nohtml=1\"); return false;' href='/previous' class='waves-effect waves-light btn'><i class='material-icons left'>skip_previous</i>Previous</a>";
    message += "  <a onclick='$.get(\"/next?nohtml=1\"); return false;' href='/next' class='waves-effect waves-light btn'><i class='material-icons left'>skip_next</i>Next</a>";
    message += "  <br/>";
    message += "  <a onclick='$.get(\"/volumedown?nohtml=1\"); return false;' href='/volumedown' class='waves-effect waves-light btn'><i class='material-icons left'>volume_down</i>-</a>";
    message += "  <a onclick='$.get(\"/volumeup?nohtml=1\"); return false;' href='/volumeup' class='waves-effect waves-light btn'><i class='material-icons left'>volume_up</i>+</a>";
    message += "</div>";
    
    message += "<div class='row'>";
    message += "  <form action='/' method='POST'>";
    message += "    <div class='row'>";
    message += "      <div class='input-field col s12'>";
    if (httpServer.arg("alarm") != "") {
      alarm = httpServer.arg("alarm");
      String sh = "";
      sh.concat(alarm[0]);
      sh.concat(alarm[1]);
  
      String sm = "";
      sm.concat(alarm[3]);
      sm.concat(alarm[4]);
      
      ah = sh.toInt();
      am = sm.toInt();

      saveFirebase();
    }    
    message += "        <input type='time' id='alarm' name='alarm' value='"+alarm+"' required>";  
    message += "      </div>";
    message += "    </div>";
    message += "    <input type='submit' text='Save' class='waves-effect waves-light btn' />";
    message += "  </form>";
    message += "</div>";
  
    message += "<div class='row'><br/>";
    message += "  <a href='/update' class='waves-effect waves-light btn'><i class='material-icons left'>cloud_upload</i>Update</a>";
    message += "</div>";
  
    message += "  </div>";
    message += "</div>";
  
    message += "</body>";
    message += "</html>";

    Serial.println("html generated");
  }

  httpServer.send(200, "text/html", message);       //Response to the HTTP request
}

void playHandler() {
  digitalWrite(PIN_LED, LOW);
  mp3_play();
  isMusic = true;
  Serial.println("mp3_play");
  indexHandler();
}

void randomHandler() {
  digitalWrite(PIN_LED, LOW);
  mp3_random_play();
  isMusic = true;
  Serial.println("mp3_random_play");
  indexHandler();
}

void pauseHandler() {
  digitalWrite(PIN_LED, HIGH);
  mp3_pause();
  isMusic = false;
  Serial.println("mp3_next");
  indexHandler();
}

void previousHandler() {
  digitalWrite(PIN_LED, LOW);
  mp3_prev();
  isMusic = true;
  Serial.println("mp3_prev");
  indexHandler();
}

void nextHandler() {
  digitalWrite(PIN_LED, LOW);
  mp3_next();
  isMusic = true;
  Serial.println("mp3_next");
  indexHandler();
}

void volumedownHandler() {
  volume--;
  if (volume < 1) volume = 1;
  mp3_set_volume(volume);
  Serial.println("mp3_set_volume down: " + String(volume));
  indexHandler();
}

void volumeupHandler() {
  volume++;
  if (volume > 30) volume = 30;
  mp3_set_volume(volume);
  Serial.println("mp3_set_volume up: " + String(volume));
  indexHandler();
}

void blink(int n) {
  digitalWrite(PIN_LED, LOW);
  delay(2000);
  
  for (int i = 0; i < n; i++) {
    digitalWrite(PIN_LED, HIGH);  
    delay(500);
    digitalWrite(PIN_LED, LOW);
    delay(500);
  }
  
  digitalWrite(PIN_LED, LOW);
}

void setup(void) {
  Serial.begin(9600);
  Serial.println();
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    Serial.println("WiFi failed, retrying.");
  }

  /*delay(100);
  IPAddress ip(192,168,1,131);   
  IPAddress gateway(192,168,1,254);   
  IPAddress subnet(255,255,255,0);   
  WiFi.config(ip, gateway, subnet);
  delay(100);*/
  
  MDNS.begin(host);
  
  localIP = WiFi.localIP();
  saveFirebaseIp();

  httpUpdater.setup(&httpServer);  
  //const char * headerkeys[] = {"User-Agent","Cookie"} ;
  //size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  //httpServer.collectHeaders(headerkeys, headerkeyssize);
  httpServer.on("/", indexHandler);
  httpServer.on("/play", playHandler);
  httpServer.on("/random", randomHandler);
  httpServer.on("/pause", pauseHandler);
  httpServer.on("/previous", previousHandler);
  httpServer.on("/next", nextHandler);
  httpServer.on("/volumedown", volumedownHandler);
  httpServer.on("/volumeup", volumeupHandler);
  httpServer.begin();

  // To upload through terminal you can use: curl -F "image=@firmware.bin" esp8266-webupdate.local/update
  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);
  Serial.println(localIP.toString());

  pinMode(PIN_BUSY, INPUT);
  mp3Serial.begin (9600);
  mp3_set_serial(mp3Serial);  
  delay(1000);
  mp3_set_volume(5);
  mp3_stop();

  getFirebase();
  mp3_set_volume(volume);
  digitalWrite(PIN_LED, HIGH);
  
  mp3_random_play();
  delay(200);
  //mp3_stop();
  mp3_pause();
}

void loop(void) {
  for (int i = 0; i < 80; i++) {
    httpServer.handleClient();
    MDNS.update();
    if (ch == ah && cm == am && !isMusic) {
      mp3_random_play();
      Serial.println("alarm time");
      isMusic = true;
    }
    delay(500);
  }
  getTimestamp();
}
