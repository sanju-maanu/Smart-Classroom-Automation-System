/*
  classroom_controller.ino
  Smart Classroom Automation - Arduino Uno variant
  - Fingerprint-based attendance (Adafruit_Fingerprint)
  - PIR motion sensor -> Relay for Light/Fan
  - Serial commands:
      'e' -> enroll new fingerprint via Serial prompts
      'd' -> delete fingerprint by ID (prompt)
      'l' -> list basic status
  Wiring (example):
    Fingerprint VCC -> 5V
    Fingerprint GND -> GND
    Fingerprint TX  -> Arduino pin 2 (SoftwareSerial RX)
    Fingerprint RX  -> Arduino pin 3 (SoftwareSerial TX)
    PIR VCC -> 5V
    PIR GND -> GND
    PIR OUT -> D7
    Relay IN -> D6
*/

#include <SoftwareSerial.h>
#include <Adafruit_Fingerprint.h>
#include <EEPROM.h>

#define FINGER_RX_PIN 2  // connect to sensor TX
#define FINGER_TX_PIN 3  // connect to sensor RX

SoftwareSerial fingerSerial(FINGER_RX_PIN, FINGER_TX_PIN); // RX, TX
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);

const int PIR_PIN = 7;
const int RELAY_PIN = 6;

unsigned long lastMotionTime = 0;
const unsigned long MOTION_TIMEOUT = 5UL * 60UL * 1000UL; // 5 minutes auto-off

// Simple attendance debounce map: store last seen time for ID to avoid duplicates (in RAM)
#define MAX_UIDS 200
unsigned long lastSeen[MAX_UIDS];

void setup() {
  Serial.begin(115200);
  while (!Serial); // for Leonardo/Micro — safe here
  Serial.println();
  Serial.println("Smart Classroom Automation - Arduino UNO");
  
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  fingerSerial.begin(57600);
  delay(100);
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor found.");
  } else {
    Serial.println("Fingerprint sensor NOT found. Check wiring & baud.");
  }

  // init lastSeen
  for (int i=0;i<MAX_UIDS;i++) lastSeen[i]=0;
}

void loop() {
  // handle motion -> relay
  handleMotion();

  // check for fingerprint press
  getFingerprintAndMarkAttendance();

  // handle serial commands
  handleSerialCommands();
}

void handleMotion() {
  int pir = digitalRead(PIR_PIN);
  if (pir == HIGH) {
    lastMotionTime = millis();
    digitalWrite(RELAY_PIN, HIGH); // turn ON appliances
  } else {
    // if no motion for timeout, turn off
    if (millis() - lastMotionTime >= MOTION_TIMEOUT) {
      digitalWrite(RELAY_PIN, LOW); // OFF
    }
  }
}

void getFingerprintAndMarkAttendance() {
  uint8_t p = finger.getImage();
  if (p == FINGERPRINT_OK) {
    // got image
    p = finger.image2Tz();
    if (p == FINGERPRINT_OK) {
      p = finger.fingerFastSearch();
      if (p == FINGERPRINT_OK) {
        uint16_t id = finger.fingerID;
        uint16_t confidence = finger.confidence;
        // debounce - only mark once every 30 seconds per id
        unsigned long t = millis();
        if (id < MAX_UIDS) {
          if (t - lastSeen[id] > 30UL * 1000UL) {
            lastSeen[id] = t;
            logAttendance(id, confidence);
          } else {
            // ignore duplicate within 30s
            Serial.print("Duplicate recently seen ID ");
            Serial.println(id);
          }
        } else {
          Serial.print("Matched ID out of range: ");
          Serial.println(id);
        }
      } else {
        Serial.println("Fingerprint not recognized.");
      }
    } else {
      Serial.println("Image to TZ failed.");
    }
  } else if (p == FINGERPRINT_NOFINGER) {
    // nothing
  } else {
    // other errors ignored
  }
}

// Log attendance - here we print to Serial; can be extended to SD/RTC/MQTT
void logAttendance(uint16_t id, uint16_t confidence) {
  Serial.print("Attendance: ID=");
  Serial.print(id);
  Serial.print(", confidence=");
  Serial.println(confidence);
  // Optionally toggle an LED or EEPROM store etc.
  // Example: blink relay briefly as visual ack (do not toggle mains relay if unsafe)
  // digitalWrite(RELAY_PIN, HIGH); delay(100); digitalWrite(RELAY_PIN, LOW);
}

// Serial handling: enroll, delete, list
void handleSerialCommands() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length()==0) return;
    if (cmd == "e" || cmd == "E") {
      Serial.println("Enrolling new fingerprint...");
      enrollFingerprint();
    } else if (cmd == "d" || cmd == "D") {
      Serial.println("Enter ID to delete (1..127):");
      while (!Serial.available()) delay(50);
      int id = Serial.parseInt();
      if (id>0) {
        if (deleteFingerprint(id)) {
          Serial.print("Deleted ID ");
          Serial.println(id);
        } else {
          Serial.print("Failed to delete ID ");
          Serial.println(id);
        }
      }
    } else if (cmd == "l" || cmd == "L") {
      Serial.println("--- Status ---");
      Serial.print("Relay state: ");
      Serial.println(digitalRead(RELAY_PIN) ? "ON" : "OFF");
      Serial.print("Last motion (ms ago): ");
      Serial.println(millis() - lastMotionTime);
      Serial.println("Type 'e' to enroll, 'd' to delete, 'l' to list status.");
    } else {
      Serial.println("Unknown command. Use e (enroll), d (delete), l (status).");
    }
  }
}

void enrollFingerprint() {
  int id = findFreeID();
  if (id < 1) {
    Serial.println("No free ID slots available.");
    return;
  }
  Serial.print("New ID will be: ");
  Serial.println(id);
  Serial.println("Place finger on sensor...");

  while (true) {
    uint8_t p = finger.getImage();
    if (p == FINGERPRINT_OK) {
      Serial.println("Image taken");
      p = finger.image2Tz(1);
      if (p != FINGERPRINT_OK) {
        Serial.println("Error converting image");
        return;
      }
      break;
    } else if (p == FINGERPRINT_NOFINGER) {
      // keep waiting
    } else {
      // handle other states
    }
    delay(200);
  }

  Serial.println("Remove finger.");
  delay(1500);

  Serial.println("Place same finger again...");
  while (true) {
    uint8_t p = finger.getImage();
    if (p == FINGERPRINT_OK) {
      Serial.println("Image taken for second sample");
      p = finger.image2Tz(2);
      if (p != FINGERPRINT_OK) {
        Serial.println("Error converting second image");
        return;
      }
      break;
    }
    delay(200);
  }

  Serial.println("Creating model...");
  uint8_t r = finger.createModel();
  if (r == FINGERPRINT_OK) {
    Serial.println("Model created");
    r = finger.storeModel(id);
    if (r == FINGERPRINT_OK) {
      Serial.println("Stored!");
      Serial.print("Enrollment successful. ID=");
      Serial.println(id);
    } else {
      Serial.print("Failed to store model, error ");
      Serial.println(r);
    }
  } else {
    Serial.print("Failed to create model, error ");
    Serial.println(r);
  }
}

int findFreeID() {
  // naive: try 1..127
  for (int i = 1; i <= 127; i++) {
    // try to load model; if not found, use this slot
    int r = finger.loadModel(i);
    if (r == FINGERPRINT_PACKETRECIEVEERR) {
      // communication error - skip
      continue;
    }
    if (r == FINGERPRINT_OK) {
      // slot used
    } else {
      // not found -> free
      return i;
    }
  }
  return -1;
}

bool deleteFingerprint(int id) {
  uint8_t r = finger.deleteModel(id);
  return (r == FINGERPRINT_OK);
}
