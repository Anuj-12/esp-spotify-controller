#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_JSON.h>
#include <HTTPClient.h>

// Spotify Controller
#define HOST_NAME "https://api.spotify.com."

// WiFi credentials 
#define SSID "wifi5"
#define PASS "12341234"

// OLED display configuration constants
#define SCREEN_WIDTH 128    // OLED display width in pixels
#define SCREEN_HEIGHT 64    // OLED display height in pixels
#define OLED_RESET -1      // Reset pin (-1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // I2C address of the OLED display

// NTP (Network Time Protocol) configuration
const char* ntpServer = "pool.ntp.org";
const long gmtOffsetSec = 19800;  // IST is UTC + 5:30 (5.5 hours * 3600 seconds)
const int dayLightOffsetSec = 0;  // No daylight savings in India

// Timing variables for display refresh
unsigned long prevMillis = 0;

// Button control variables
unsigned long buttPrevMill = 0;  // For button debouncing
int checkCount = 0;              // Display refresh counter
int buttPin = 2;                 // Mode selection button pin
int mode = 0;                    // Current display mode

// Function button variables
unsigned long funcPrevMill = 0;
int funcPin = 25;               // Function button pin
int funcMode = 0;               // Function mode state

// Timer and potentiometer variables
unsigned long potMill = 0;       // For potentiometer reading intervals
unsigned long duckingMill = 0;   // For timer countdown
unsigned long funcMill = 0;      // For function button debouncing
bool timerOn = false;           // Timer state
int hourVal = 0;                // Timer hours
int minVal = 0;                 // Timer minutes
int potPin = 33;                // Potentiometer input pin
int potVal = 0;                 // Current potentiometer value

// Buzzer pin for timer alarm
int buzzPin = 14;

// Create the OLED display object
WiFiClient client;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// HTTP Client

/**
 * Initial setup function
 * - Initializes serial communication
 * - Sets up OLED display
 * - Connects to WiFi network
 * - Configures NTP for time synchronization
 * - Sets up GPIO pins for buttons, potentiometer, and buzzer
 */
void setup(){
  Serial.begin(9600);

  // Initialize OLED display with error checking
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)){
    Serial.println("SSD1306 allocation failed!");
    // loop forever if display initialization fails
    while (true){}
  }

  // Configure WiFi in station mode and disconnect from any previous connections
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Display WiFi connection status on OLED
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Waiting \nfor WiFi \nconnection");
  display.display();
  
  // Connect to WiFi with status updates
  Serial.println("*******************************************************");
  Serial.print("Attempting to connect to the WiFi network : "); Serial.println(SSID);
  WiFi.begin(SSID, PASS);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected successfully!");

  // Initialize NTP time synchronization
  configTime(gmtOffsetSec, dayLightOffsetSec, ntpServer);

  // Configure GPIO pins
  pinMode(buttPin, INPUT_PULLDOWN);      // Mode selection button
  pinMode(potPin, INPUT);                // Potentiometer for timer setting
  pinMode(buzzPin, OUTPUT);              // Buzzer output
  pinMode(funcPin, INPUT_PULLDOWN);      // Function button
  digitalWrite(buzzPin, LOW);            // Ensure buzzer is off initially
}

/**
 * Displays the current time and date on the OLED screen
 * Updates every minute and formats the display with:
 * - Hours and minutes in 12-hour format
 * - AM/PM indicator
 * - Date and day of the week
 */
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

/**
 * Handles the timer functionality
 * - Uses potentiometer input to set timer duration
 * - Displays countdown when timer is active
 * - Triggers buzzer when timer reaches zero
 * - Updates display every minute
 */
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

/**
 * Main program loop
 * - Handles button inputs for mode switching
 * - Manages timer state
 * - Updates display based on current mode
 * - Reads potentiometer value for timer setting
 */
void loop(){

  int buttonState = digitalRead(buttPin);
  int funcState = digitalRead(funcPin);
  Serial.print(funcState);Serial.print(" ");Serial.println(buttonState);
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

void spotifyController(){
  HTTPClient http;
  http.begin(client, HOST_NAME);
}