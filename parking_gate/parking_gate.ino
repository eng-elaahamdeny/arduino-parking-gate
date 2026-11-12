/*
  Arduino parking gate: two HC-SR04 sensors, one SG90 servo, 16x2 I2C LCD.

  Behaviour (same idea as the tutorial credited in README.md):
    - Car at the ENTRY sensor and a slot is free -> gate opens.
    - Once the car has reached and then cleared the EXIT sensor -> gate closes, free slots -1.
    - Car at the EXIT sensor first -> gate opens.
    - Once the car has reached and then cleared the ENTRY sensor -> gate closes, free slots +1.

  This sketch was written from scratch (state machine), not copied from the tutorial.
*/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// ---------------- Configuration ----------------
const uint8_t  TRIG_ENTRY = 6, ECHO_ENTRY = 7;
const uint8_t  TRIG_EXIT  = 8, ECHO_EXIT  = 9;
const uint8_t  SERVO_PIN  = 2;
const uint8_t  LCD_ADDR   = 0x27;

const uint8_t  TOTAL_SLOTS    = 4;       // change for a bigger car park
const int      DETECT_CM      = 10;      // a car is "present" below this distance
const uint8_t  CONFIRM_READS  = 2;       // consecutive readings needed (anti-glitch)
const uint8_t  GATE_CLOSED    = 100;     // servo angles (degrees)
const uint8_t  GATE_OPEN      = 0;
const unsigned long PASS_TIMEOUT_MS = 10000;  // close anyway if no car passes
const unsigned long ECHO_TIMEOUT_US = 25000;  // ~4 m max range
const unsigned long FULL_MSG_MS     = 2000;

// ---------------- Globals ----------------
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
Servo gate;

struct Detector {
  uint8_t trig, echo;
  uint8_t hits;      // consecutive "near" readings
  bool    present;   // debounced result
};
Detector entrySensor = {TRIG_ENTRY, ECHO_ENTRY, 0, false};
Detector exitSensor  = {TRIG_EXIT,  ECHO_EXIT,  0, false};

enum State { IDLE, ENTERING, EXITING };
State state = IDLE;
uint8_t freeSlots = TOTAL_SLOTS;
bool reachedSecond = false;          // car has reached the 2nd sensor
unsigned long stateStart = 0;
unsigned long fullMsgUntil = 0;

// ---------------- Helpers ----------------
// Distance in cm, or -1 if no echo came back.
// (pulseIn returns 0 on timeout: that must NOT be read as "0 cm".)
int readDistanceCm(uint8_t trig, uint8_t echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  unsigned long us = pulseIn(echo, HIGH, ECHO_TIMEOUT_US);
  if (us == 0) return -1;
  return (int)(us * 0.034 / 2);
}

void updateDetector(Detector &d) {
  int cm = readDistanceCm(d.trig, d.echo);
  bool near = (cm > 0 && cm < DETECT_CM);
  if (near) { if (d.hits < CONFIRM_READS) d.hits++; }
  else d.hits = 0;
  d.present = (d.hits >= CONFIRM_READS);
}

void moveGate(uint8_t from, uint8_t to) {
  int step = (from < to) ? 1 : -1;
  for (int p = from; p != to; p += step) { gate.write(p); delay(5); }
  gate.write(to);
}

void printLine(uint8_t row, const char *text) {
  char buf[17];
  snprintf(buf, sizeof(buf), "%-16s", text);
  lcd.setCursor(0, row);
  lcd.print(buf);
}

void showStatus() {
  char line[17];
  if (state == ENTERING)       { printLine(0, "Gate open");    printLine(1, "Please enter"); return; }
  if (state == EXITING)        { printLine(0, "Gate open");    printLine(1, "Please exit");  return; }
  if (millis() < fullMsgUntil) { printLine(0, "Parking full"); printLine(1, "Sorry!");       return; }
  printLine(0, "Parking gate");
  snprintf(line, sizeof(line), "Free slots: %u", freeSlots);
  printLine(1, line);
}

void openGate(State next) {
  moveGate(GATE_CLOSED, GATE_OPEN);
  state = next;
  reachedSecond = false;
  stateStart = millis();
  Serial.println(next == ENTERING ? F("Gate open (entry)") : F("Gate open (exit)"));
}

void closeGate(bool carPassed) {
  moveGate(GATE_OPEN, GATE_CLOSED);
  if (carPassed) {
    if (state == ENTERING && freeSlots > 0)                freeSlots--;
    else if (state == EXITING && freeSlots < TOTAL_SLOTS)  freeSlots++;
  }
  Serial.print(F("Gate closed, free slots = "));
  Serial.println(freeSlots);
  state = IDLE;
}

// ---------------- Arduino entry points ----------------
void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  pinMode(TRIG_ENTRY, OUTPUT); pinMode(ECHO_ENTRY, INPUT);
  pinMode(TRIG_EXIT,  OUTPUT); pinMode(ECHO_EXIT,  INPUT);
  gate.attach(SERVO_PIN);
  gate.write(GATE_CLOSED);
  printLine(0, "Parking gate");
  printLine(1, "Starting...");
  delay(1500);
}

void loop() {
  updateDetector(entrySensor);
  delay(30);                       // let echoes die out between the two sensors
  updateDetector(exitSensor);

  switch (state) {
    case IDLE:
      if (entrySensor.present) {
        if (freeSlots > 0) openGate(ENTERING);
        else fullMsgUntil = millis() + FULL_MSG_MS;
      } else if (exitSensor.present && freeSlots < TOTAL_SLOTS) {
        openGate(EXITING);
      }
      break;

    case ENTERING:
    case EXITING: {
      const Detector &second = (state == ENTERING) ? exitSensor : entrySensor;
      if (second.present) reachedSecond = true;
      if (reachedSecond && !second.present)               closeGate(true);   // car went through
      else if (millis() - stateStart > PASS_TIMEOUT_MS)   closeGate(false);  // nobody came
      break;
    }
  }

  showStatus();
  delay(50);
}
