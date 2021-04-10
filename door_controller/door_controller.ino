#define BLYNK_PRINT Serial

#define BLYNK_TIMEOUT_MS  500  // must be BEFORE BlynkSimpleEsp8266.h doesn't work !!!
#define BLYNK_HEARTBEAT   17   // must be BEFORE BlynkSimpleEsp8266.h works OK as 17s

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include "secrets.h"

#define BLYNK_INPUT_PIN V1
#define BLYNK_DOOR_STATE_PIN V2
#define BLYNK_COUNTDOWN_PIN V3
#define BUTTON_PIN 13
#define OPTOCOUPLER_PIN 15
#define DEFAULT_DURATION 20

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = SECRET_AUTH_TOKEN;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

BlynkTimer timer;

bool on = false;
bool online = false;

int countdown = 0;
int countdownTimerId;

void closeDoor() {
  Serial.println("close a door");
  
  if (timer.isEnabled(countdownTimerId)) {
    timer.deleteTimer(countdownTimerId);
  }
  
  Blynk.virtualWrite(BLYNK_COUNTDOWN_PIN, 0);
  Blynk.virtualWrite(BLYNK_INPUT_PIN, 0);
  
  countdown = 0;
  
  Blynk.virtualWrite(BLYNK_DOOR_STATE_PIN, false);
  digitalWrite(LED_BUILTIN, false);
  digitalWrite(OPTOCOUPLER_PIN, false);
}

void updateCountdown() {
  countdown--;
  Serial.print("countdown: ");
  Serial.println(countdown);
  Blynk.virtualWrite(BLYNK_COUNTDOWN_PIN, countdown);
  
}

void openDoorFor(int duration = DEFAULT_DURATION) {
  closeDoor();
  
  Serial.print("open a door for: ");
  Serial.println(duration);
  
  Blynk.virtualWrite(BLYNK_COUNTDOWN_PIN, duration);
  Blynk.virtualWrite(BLYNK_INPUT_PIN, duration);
  
  countdown = duration;
  
  Blynk.virtualWrite(BLYNK_DOOR_STATE_PIN, true);
  digitalWrite(LED_BUILTIN, true);
  digitalWrite(OPTOCOUPLER_PIN, true);

  timer.setTimeout(duration * 1000L, closeDoor);
  countdownTimerId = timer.setTimer(1000, updateCountdown, duration);
}


BLYNK_WRITE(BLYNK_INPUT_PIN) {
  int pinValue = param.asInt(); // assigning incoming value from pin to a variable
  if (pinValue) {
    openDoorFor(pinValue);
  } else {
    closeDoor();
  }
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(OPTOCOUPLER_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Debug console
  Serial.begin(115200);
  Serial.println("setup");

  // WiFi
  WiFi.hostname(WIFI_HOSTNAME);
  WiFi.mode(WIFI_STA);

  // Blynk
  Blynk.config(auth);

  checkConnection(); // It needs to run first to initiate the connection.
  //Same function works for checking the connection!
  timer.setInterval(5000L, checkConnection);
}

void loop() {
  if (Blynk.connected()) {
    Blynk.run();
  }
  timer.run();
  
  if (!digitalRead(BUTTON_PIN) && countdown < DEFAULT_DURATION) {
    openDoorFor(DEFAULT_DURATION);
  }
}

void checkConnection() {
  if (!Blynk.connected()) {
    online = 0;
    yield();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Not connected to Wifi! Connect...");
      Blynk.connectWiFi(ssid, pass);
      delay(400); //give it some time to connect
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Cannot connect to WIFI!");
        online = 0;
      } else {
        Serial.println("Connected to wifi!");
      }
    }
    
    if ( WiFi.status() == WL_CONNECTED && !Blynk.connected() ) {
      Serial.println("Not connected to Blynk Server! Connecting..."); 
      Blynk.connect();  // // It has 3 attempts of the defined BLYNK_TIMEOUT_MS to connect 
      // to the server, otherwise it goes to the enxt line 
      if (!Blynk.connected()) {
        Serial.println("Connection failed!"); 
        online = 0;
      } else {
        online = 1;
      }
    }
  } else {
    Serial.println("Connected to Blynk server!"); 
    online = 1;    
  }
}
