#include <Arduino.h>
#include <Wire.h>
#include "sensing_logic.h"
#include "slave_link.h"

// Set to the assembled wiring. Reeds are normally open to ground with the internal pull-up.
static const uint8_t REED1_PIN = 2;
static const uint8_t REED2_PIN = 3;
static const uint8_t LED_GREEN_PIN = 5;
static const uint8_t LED_AMBER_PIN = 6;
static const uint8_t LED_RED_PIN = 7;
static const uint16_t DEBOUNCE_MS = 20;
static const uint16_t SAMPLE_INTERVAL_MS = 2;

static SensingLogic logic;
static SlaveLink link;
static uint32_t lastSampleMs = 0;
static uint32_t lastPublishMs = 0;

static bool reedClosed(uint8_t pin) { return digitalRead(pin) == LOW; }

void setup() {
  Serial.begin(9600);
  pinMode(REED1_PIN, INPUT_PULLUP);
  pinMode(REED2_PIN, INPUT_PULLUP);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_AMBER_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  logic.begin(DEBOUNCE_MS);
  link.begin(SENSING_ADDRESS, SENSING_REPLY_LENGTH);
  Serial.print(F("SENSING contract v"));
  Serial.println(CONTRACT_VERSION);
}

void loop() {
  uint32_t now = millis();
  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    logic.sample(now, reedClosed(REED1_PIN), reedClosed(REED2_PIN));
  }
  logic.serviceWatchdog(link.contactLost(now));
  digitalWrite(LED_GREEN_PIN, logic.greenOn());
  digitalWrite(LED_AMBER_PIN, logic.amberOn());
  digitalWrite(LED_RED_PIN, logic.redOn());
  if (now - lastPublishMs >= SNAPSHOT_REFRESH_MS) {
    lastPublishMs = now;
    uint8_t bytes[SENSING_REPLY_LENGTH];
    logic.encode(bytes, now);
    link.publish(bytes);
  }
}
