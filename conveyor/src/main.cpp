#include <Arduino.h>
#include <Wire.h>
#include <Stepper.h>
#include "conveyor_logic.h"
#include "slave_link.h"

// Motor section: replace with the proven library, pins and speed from the working sketch.
// Keep the three hooks below; the logic and link do not care how steps are made.
static const int STEPS_PER_REVOLUTION = 2048;
static const int MOTOR_PIN_1 = 8;
static const int MOTOR_PIN_2 = 10;
static const int MOTOR_PIN_3 = 9;
static const int MOTOR_PIN_4 = 11;
static const uint32_t STEP_INTERVAL_MICROS = 2000;

static Stepper stepper(STEPS_PER_REVOLUTION, MOTOR_PIN_1, MOTOR_PIN_2, MOTOR_PIN_3, MOTOR_PIN_4);
static uint32_t lastStepMicros = 0;

static void motorService(MotorRequest request, ConveyorLogic& logic) {
  if (request == MOTOR_OFF) return;
  uint32_t now = micros();
  if (now - lastStepMicros < STEP_INTERVAL_MICROS) return;
  lastStepMicros = now;
  stepper.step(request == MOTOR_FORWARD ? 1 : -1);
  logic.countStep();
}

static ConveyorLogic logic;
static SlaveLink link;
static uint32_t lastPublishMs = 0;

void setup() {
  Serial.begin(9600);
  logic.begin();
  link.begin(CONVEYOR_ADDRESS, CONVEYOR_REPLY_LENGTH);
  Serial.print(F("CONVEYOR contract v"));
  Serial.println(CONTRACT_VERSION);
}

void loop() {
  uint32_t now = millis();
  char command = link.takeCommand();
  if (command) logic.command(command);
  logic.serviceWatchdog(link.contactLost(now));
  motorService(logic.motor(), logic);
  if (now - lastPublishMs >= SNAPSHOT_REFRESH_MS) {
    lastPublishMs = now;
    uint8_t bytes[CONVEYOR_REPLY_LENGTH];
    logic.encode(bytes, now);
    link.publish(bytes);
  }
}
