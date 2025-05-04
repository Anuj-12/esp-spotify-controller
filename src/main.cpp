#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi credentials 
#define SSID "Anuj's Galaxy M31"
#define PASS "VadaPavGr8"

// OLED properties
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Open source Network Time Protocol server
const char* ntpServer = "pool.ntp.org";
const long gmtOffsetSec = 19800;  // IST is UTC + 5:30
const int dayLightOffsetSec = 0;  // None for India

unsigned long prevMillis = 0;

unsigned long buttPrevMill = 0;
int checkCount = 0;
int buttPin = 2;
int mode = 0;

unsigned long funcPrevMill = 0;
int funcPin = 25; 
int funcMode = 0;

unsigned long potMill = 0;
unsigned long duckingMill = 0;
unsigned long funcMill = 0;
bool timerOn = false;
int hourVal = 0;
int minVal = 0;
int potPin = 33;
int potVal = 0;
int buzzPin = 14;


// Created the oled object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup(){
  Serial.begin(9600);

  // Initializing the OLED object
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)){
    Serial.println("SSD1306 allocation failed!");
    // loop forever
    while (true){}
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Waiting \nfor WiFi \nconnection");
  display.display();
  
  Serial.println("*******************************************************");
  Serial.print("Attempting to connect to the WiFi network : "); Serial.println(SSID);
  WiFi.begin(SSID, PASS);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected successfully!");

  // Set time according to the NTP server 
  configTime(gmtOffsetSec, dayLightOffsetSec, ntpServer);


  pinMode(buttPin, INPUT_PULLDOWN);
  pinMode(potPin, INPUT);
  pinMode(buzzPin, OUTPUT);
  pinMode(funcPin, INPUT_PULLDOWN);
  digitalWrite(buzzPin, LOW);
}

void displayTime(){
  unsigned long currentMillis = millis();
  struct tm timeInfo;
  if(!getLocalTime(&timeInfo)){
    Serial.println("Failed to recover time!");
    return;
  }

  int milliSecLeft = (60 - timeInfo.tm_sec) * 1000;

  if(currentMillis - prevMillis >= milliSecLeft || checkCount == 0){
    prevMillis = currentMillis;
    // println(tm *timeinfo, const char *format = (const char *)__null)
    // %I -> hours in 12 hr format
    // %M -> Minutes
    // %p -> am or pm
    display.clearDisplay();
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.print(&timeInfo, "%I:%M");
    display.setTextSize(2);
    display.setCursor(100,8);
    display.print(&timeInfo, "%p");
    display.setCursor(0, 30);
    display.setTextSize(2);
    display.print(&timeInfo, "%d %B \n%A");
    display.display();
    
    if(checkCount == 0) checkCount++;
  }
}

void displayTimer(){

  unsigned long currentMillis = millis();

  Serial.println(duckingMill);
  display.clearDisplay();
  display.setCursor(0,30);
  display.setTextColor(WHITE);

  if(timerOn == false){
    minVal = potVal * (240. / 4095.);
    minVal = (minVal % 5 == 0) ?  minVal : minVal - (minVal % 5);
    hourVal = minVal / 60;

    if(potVal == 0){
      display.setTextSize(2);
      display.print("Timer mode");
    } else{
      display.clearDisplay();
      if(minVal < 60){
        display.setTextSize(3);
        display.setCursor(40,30);
        display.print(minVal);
      }else {
        display.setTextSize(3);
        display.setCursor(30,30);
        if(minVal < 60){
          display.print("0"); display.print(minVal);
        }else{
          display.print(hourVal); display.print(":");
          if(minVal % 60 < 10){
            display.print("0"); display.print(minVal % 60);
          }else{
            display.print(minVal % 60);
          }
        }
      }
    }

    display.display();
    checkCount++;
  }else{
    display.setTextSize(3);
    display.setCursor(30,30);
    if(minVal < 60){
      display.print("0"); display.print(minVal);
    }else{
      display.print(hourVal); display.print(":");
      if(minVal % 60 < 10){
        display.print("0"); display.print(minVal % 60);
      }else{
        display.print(minVal % 60);
      }
    }
    display.display();
    if(currentMillis - duckingMill >= 60 * 1000){
      duckingMill = currentMillis;
      minVal -= 1;
      Serial.println("Min value should be subtracted!!");
      if(minVal % 60 == 0) hourVal = minVal / 60;
    }
    if(minVal == 0 && timerOn){
      digitalWrite(buzzPin, HIGH);
      display.clearDisplay();
      if(currentMillis - prevMillis < 500) display.print("Time up!");
    }
  }

  display.display();
  Serial.println(minVal);
}

void loop(){

  int buttonState = digitalRead(buttPin);
  int funcState = digitalRead(funcPin);
  unsigned long buttCurrMill = millis();

  if(!timerOn){
    digitalWrite(buzzPin, LOW);
  }

  if(buttCurrMill - potMill >= 300){
    potMill = buttCurrMill;
    potVal = analogRead(potPin);
  }

  if(buttCurrMill - funcMill >= 300 && funcState == 1){
    funcMill = buttCurrMill;
    funcMode++;
    if(funcMode > 1) funcMode = 0;
    if(funcMode == 0) timerOn = false;
    else timerOn = true;
  }

  if(mode == 0){
    displayTime();
  }else if(mode == 1){
    displayTimer();
  }else{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.print("Work in \nprogress..");
    display.display();
  }

  if(buttCurrMill - buttPrevMill >= 350 && buttonState == 1){

    buttPrevMill = buttCurrMill;
    mode++;
    prevMillis = 0;
    checkCount = 0;

    if(mode > 2){
      mode = 0;
    }
  }
}