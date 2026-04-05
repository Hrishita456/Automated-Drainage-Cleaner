/*
  Automated Drainage Cleaner
  Arduino control code
  Supports:
  - Motor control for rake + conveyor
  - Bluetooth/manual commands
  - CV-triggered serial commands
  - Automatic timed cleaning cycle

  Assumed driver: L298N
*/

#include <SoftwareSerial.h>

// ===================== BLUETOOTH =====================
const int BT_RX = 10;   // Arduino receives on pin 10
const int BT_TX = 11;   // Arduino transmits on pin 11
SoftwareSerial BT(BT_RX, BT_TX);

// ===================== MOTOR PINS =====================
// Motor 1: Rake / cleaner mechanism
const int RAKE_IN1 = 2;
const int RAKE_IN2 = 3;
const int RAKE_EN  = 5;   // PWM

// Motor 2: Conveyor
const int CONV_IN1 = 4;
const int CONV_IN2 = 7;
const int CONV_EN  = 6;   // PWM

// Optional buzzer / status LED
const int STATUS_LED = 13;

// ===================== SETTINGS =====================
int rakeSpeed = 180;       // 0 to 255
int conveyorSpeed = 200;   // 0 to 255

bool autoMode = true;
bool cleaningActive = false;

// Timed cycle parameters
unsigned long cleaningStartTime = 0;
const unsigned long CLEANING_DURATION = 8000; // 8 seconds total cycle

String inputBuffer = "";

// ===================== MOTOR HELPERS =====================
void stopMotor(int in1, int in2, int en) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  analogWrite(en, 0);
}

void motorForward(int in1, int in2, int en, int speedVal) {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  analogWrite(en, speedVal);
}

void motorReverse(int in1, int in2, int en, int speedVal) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  analogWrite(en, speedVal);
}

void stopAll() {
  stopMotor(RAKE_IN1, RAKE_IN2, RAKE_EN);
  stopMotor(CONV_IN1, CONV_IN2, CONV_EN);
  cleaningActive = false;
  digitalWrite(STATUS_LED, LOW);
  Serial.println("ACK:STOPPED");
  BT.println("ACK:STOPPED");
}

void rakeOn() {
  motorForward(RAKE_IN1, RAKE_IN2, RAKE_EN, rakeSpeed);
  Serial.println("ACK:RAKE_ON");
  BT.println("ACK:RAKE_ON");
}

void rakeOff() {
  stopMotor(RAKE_IN1, RAKE_IN2, RAKE_EN);
  Serial.println("ACK:RAKE_OFF");
  BT.println("ACK:RAKE_OFF");
}

void conveyorOn() {
  motorForward(CONV_IN1, CONV_IN2, CONV_EN, conveyorSpeed);
  Serial.println("ACK:CONVEYOR_ON");
  BT.println("ACK:CONVEYOR_ON");
}

void conveyorOff() {
  stopMotor(CONV_IN1, CONV_IN2, CONV_EN);
  Serial.println("ACK:CONVEYOR_OFF");
  BT.println("ACK:CONVEYOR_OFF");
}

void startCleaningCycle() {
  cleaningActive = true;
  cleaningStartTime = millis();

  // Start both mechanisms
  rakeOn();
  conveyorOn();

  digitalWrite(STATUS_LED, HIGH);

  Serial.println("ACK:CLEANING_STARTED");
  BT.println("ACK:CLEANING_STARTED");
}

void updateCleaningCycle() {
  if (!cleaningActive) return;

  unsigned long elapsed = millis() - cleaningStartTime;

  if (elapsed >= CLEANING_DURATION) {
    stopAll();
    Serial.println("ACK:CLEANING_DONE");
    BT.println("ACK:CLEANING_DONE");
  }
}

// ===================== COMMAND PARSER =====================
void executeCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  if (cmd.length() == 0) return;

  // Mode control
  if (cmd == "AUTO_ON" || cmd == "A") {
    autoMode = true;
    Serial.println("ACK:AUTO_MODE_ON");
    BT.println("ACK:AUTO_MODE_ON");
    return;
  }

  if (cmd == "AUTO_OFF" || cmd == "M") {
    autoMode = false;
    Serial.println("ACK:MANUAL_MODE_ON");
    BT.println("ACK:MANUAL_MODE_ON");
    return;
  }

  // Stop
  if (cmd == "STOP_ALL" || cmd == "S") {
    stopAll();
    return;
  }

  // Manual motor control
  if (cmd == "CONVEYOR_ON" || cmd == "C") {
    conveyorOn();
    return;
  }

  if (cmd == "CONVEYOR_OFF" || cmd == "X") {
    conveyorOff();
    return;
  }

  if (cmd == "RAKE_ON" || cmd == "R") {
    rakeOn();
    return;
  }

  if (cmd == "RAKE_OFF" || cmd == "T") {
    rakeOff();
    return;
  }

  // Cleaning trigger
  if (cmd == "START_CLEAN" || cmd == "F") {
    if (autoMode || cmd == "F") {
      startCleaningCycle();
    }
    return;
  }

  // Status
  if (cmd == "STATUS") {
    Serial.print("STATUS:autoMode=");
    Serial.print(autoMode);
    Serial.print(", cleaningActive=");
    Serial.println(cleaningActive);

    BT.print("STATUS:autoMode=");
    BT.print(autoMode);
    BT.print(", cleaningActive=");
    BT.println(cleaningActive);
    return;
  }

  Serial.print("ERR:UNKNOWN_CMD:");
  Serial.println(cmd);
  BT.print("ERR:UNKNOWN_CMD:");
  BT.println(cmd);
}

// Read line-based input from a serial stream
void readFromStream(Stream &streamObj) {
  while (streamObj.available()) {
    char c = streamObj.read();

    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        executeCommand(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
}

// ===================== SETUP =====================
void setup() {
  pinMode(RAKE_IN1, OUTPUT);
  pinMode(RAKE_IN2, OUTPUT);
  pinMode(RAKE_EN, OUTPUT);

  pinMode(CONV_IN1, OUTPUT);
  pinMode(CONV_IN2, OUTPUT);
  pinMode(CONV_EN, OUTPUT);

  pinMode(STATUS_LED, OUTPUT);

  Serial.begin(9600);
  BT.begin(9600);

  stopAll();

  Serial.println("Automated Drainage Cleaner Ready");
  BT.println("Automated Drainage Cleaner Ready");
}

// ===================== LOOP =====================
void loop() {
  readFromStream(Serial); // CV / USB serial commands
  readFromStream(BT);     // Bluetooth app commands
  updateCleaningCycle();
}