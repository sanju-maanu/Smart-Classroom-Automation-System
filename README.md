# Smart Classroom Automation System

## Overview
The Smart Classroom Automation System automates attendance using a fingerprint sensor and controls lights and fans using motion sensors. The system improves classroom security and reduces electricity consumption by automatically turning appliances ON/OFF.

This project is suitable for schools, colleges, labs, and training centers.

---

## Features
- Fingerprint-based attendance system
- Motion detection using PIR/IR sensors
- Automatic ON/OFF control of lights and fans
- Energy-saving operation
- Separate 5V (logic) and 12V (load) power rails
- Optional IoT support using ESP32

---

## System Architecture
Fingerprint Sensor → Microcontroller → Attendance Log  
Motion Sensor → Microcontroller → Relay Module → Light/Fan

(Place your diagram image here)

---

## Hardware Required
- Arduino Uno or ESP32  
- Fingerprint Sensor (R305 / AS608)  
- PIR or IR Motion Sensor  
- Relay Module (1, 2, or 4 channel)  
- Light / Fan (AC or DC depending on prototype)  
- 5V & 12V Power Supply  
- Jumper Wires  

---

## Circuit Connections

### Fingerprint Sensor
VCC → 5V  
GND → GND  
TX → D2 (SoftwareSerial RX)  
RX → D3 (SoftwareSerial TX)

### PIR Motion Sensor
VCC → 5V  
GND → GND  
OUT → D7

### Relay Module
VCC → 5V  
GND → GND  
IN → D6  
COM → Appliance Phase  
NO → Light/Fan

---

## Working

### 1. Attendance System
1. User places finger on fingerprint sensor  
2. Controller verifies fingerprint  
3. Attendance is marked and stored  
4. (Optional) Data can be uploaded to cloud dashboard  

### 2. Motion-Based Automation
- If motion is detected → Lights/Fans turn ON  
- If no motion for a set time → Lights/Fans turn OFF  

This reduces unnecessary power consumption.

---

## Sample Arduino Code
```cpp
#include <SoftwareSerial.h>
#include <Adafruit_Fingerprint.h>

SoftwareSerial fingerSerial(2, 3);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);

int relayPin = 6;
int pirPin = 7;

void setup() {
  Serial.begin(9600);
  pinMode(relayPin, OUTPUT);
  pinMode(pirPin, INPUT);
  finger.begin(57600);
}

void loop() {
  // Motion-controlled relay
  if (digitalRead(pirPin)) {
    digitalWrite(relayPin, HIGH);
  } else {
    digitalWrite(relayPin, LOW);
  }

  // Fingerprint logic can be added here
}
