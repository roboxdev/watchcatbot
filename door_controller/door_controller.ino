#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include "secrets.h"

#define BLYNK_INPUT_PIN V1
#define BLYNK_DOOR_STATE_PIN V2
#define BLYNK_COUNTDOWN_PIN V3
#define BUTTON_PIN 13
#define OPTOCOUPLER_PIN 15
#define LED_PIN 2
#define DEFAULT_DURATION 20

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = SECRET_AUTH_TOKEN;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

unsigned long holdingDoorSince = 0;
int countdown = 0;
int doorOpeningDuration = 0;

void setDoorState(bool doorState) {
  Blynk.virtualWrite(BLYNK_DOOR_STATE_PIN, doorState);
  digitalWrite(LED_PIN, doorState);
  digitalWrite(OPTOCOUPLER_PIN, doorState);
}

void closeDoor() {
  Serial.println("close a door");
  
  Blynk.virtualWrite(BLYNK_COUNTDOWN_PIN, 0);
  Blynk.virtualWrite(BLYNK_INPUT_PIN, 0);
  
  holdingDoorSince = 0;
  countdown = 0;
  doorOpeningDuration = 0;
  
  setDoorState(false); 
}

void openDoorFor(int duration) {
  closeDoor();
  
  Serial.print("open a door for: ");
  Serial.println(duration);
  
  Blynk.virtualWrite(BLYNK_COUNTDOWN_PIN, duration);
  Blynk.virtualWrite(BLYNK_INPUT_PIN, duration);
  
  holdingDoorSince = millis();
  countdown = duration;
  doorOpeningDuration = duration;
  
  setDoorState(true); 
}

void updateCountdown() {
  int newCountdown = doorOpeningDuration - ((millis() - holdingDoorSince) / 1000);
  if (newCountdown <= 0) {
    closeDoor();
  }
  if (newCountdown != countdown) {
    Serial.print("countdown: ");
    Serial.println(newCountdown);
    Blynk.virtualWrite(BLYNK_COUNTDOWN_PIN, newCountdown);
    countdown = newCountdown;
  }
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
  pinMode(LED_PIN, OUTPUT);
  
  // Debug console
  Serial.begin(115200);
  Serial.println("setup");
  
  Blynk.begin(auth, ssid, pass);

  openDoorFor(DEFAULT_DURATION);
}

void loop() {
  if (countdown) {
    updateCountdown();    
  }
  if (!digitalRead(BUTTON_PIN) && countdown < DEFAULT_DURATION) {
    openDoorFor(DEFAULT_DURATION);
  }
  Blynk.run();
}
