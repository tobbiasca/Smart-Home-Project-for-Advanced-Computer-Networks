//VERSION 1.0

#include <Keypad.h>
#include <DHT.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

#define BLYNK_PRINT Serial

// ------------------------------------
// WIFI & BLYNK
// ------------------------------------
char ssid[] = "NIRI";
char pass[] = "123454321";
char auth[] = "17OZLEEf8nnmzWdaWwVZFvu7uE9Zdyas";
BlynkTimer timer;

// ------------------------------------
// KEYPAD
// ------------------------------------
const byte ROWS = 2;
const byte COLS = 2;
char keys[ROWS][COLS] = {
  {'1','2'},
  {'4','5'}
};
byte rowPins[ROWS] = {13, 12};
byte colPins[COLS] = {14, 27};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String inputPassword = "";
const String correctPassword = "1111";
int v0State = 0;

// ------------------------------------
// DHT SENSOR
// ------------------------------------
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ------------------------------------
// ANALOG SENSORS
// ------------------------------------
const int lightPin = 35;
const int flamePin = 32;
const int laserPin = 33;

// ------------------------------------
// JOYSTICK
// ------------------------------------
#define JOY_Y_PIN 36
#define JOY_SW_PIN 34

int joystickValue = 128;        // 0–255
int lastJoySent = -1;
int joyToggle = 0;

const int JOY_STEP = 10;        // Increment/decrement per move
const int JOY_DEADZONE_UP = 3500;
const int JOY_DEADZONE_DOWN = 800;
const unsigned long JOY_INTERVAL = 80; // ms
unsigned long lastJoyMove = 0;

// ------------------------------------
// OVERRIDE BUTTON
// ------------------------------------
const int OVERRIDE_BUTTON_PIN = 2;
int overrideState = 0;

// ------------------------------------
// BLYNK WRITE V0
// ------------------------------------
BLYNK_WRITE(V0) {
  v0State = param.asInt();
}

// ------------------------------------
// HELPER FUNCTIONS
// ------------------------------------

// Read joystick vertical axis and update V5
void readJoystick() {
  unsigned long now = millis();
  int y = analogRead(JOY_Y_PIN);

  if (now - lastJoyMove >= JOY_INTERVAL) {
    if (y > JOY_DEADZONE_UP) joystickValue += JOY_STEP;
    else if (y < JOY_DEADZONE_DOWN) joystickValue -= JOY_STEP;

    joystickValue = constrain(joystickValue, 0, 255);

    if (joystickValue != lastJoySent) {
      Blynk.virtualWrite(V5, joystickValue);
      lastJoySent = joystickValue;
    }

    lastJoyMove = now;
  }
}

// Handle joystick button to toggle V6
void readJoystickButton() {
  static int lastReading = HIGH;
  static unsigned long lastChange = 0;
  const unsigned long debounce = 50;

  int reading = digitalRead(JOY_SW_PIN);
  if (reading != lastReading) lastChange = millis();

  if ((millis() - lastChange) > debounce && reading != lastReading) {
    if (reading == LOW) {
      joyToggle = !joyToggle;
      Blynk.virtualWrite(V6, joyToggle);
    }
  }

  lastReading = reading;
}

// Handle override button to toggle V7
void handleOverrideButtonPress() {
  static int lastReading = HIGH;
  static unsigned long lastChange = 0;
  const unsigned long debounce = 50;

  int reading = digitalRead(OVERRIDE_BUTTON_PIN);
  if (reading != lastReading) lastChange = millis();

  if ((millis() - lastChange) > debounce && reading != lastReading) {
    if (reading == LOW) {
      overrideState = !overrideState;
      Blynk.virtualWrite(V7, overrideState);
    }
  }

  lastReading = reading;
}

// Password input via keypad
void handlePasswordInput(char key) {
  inputPassword += key;
  if (inputPassword.length() == 4) {
    if (v0State == 0 && inputPassword == correctPassword) {
      Blynk.virtualWrite(V0, 1);
    }
    inputPassword = "";
  }
}

// Sensor readings
void readLightSensor() { if (v0State == 1) Blynk.virtualWrite(V3, analogRead(lightPin)); }
void readLaserSensor() { Blynk.virtualWrite(V8, analogRead(laserPin) < 20 ? 1 : 0); }
void readFlameSensor() { Blynk.virtualWrite(V1, analogRead(flamePin)); }
void readDHT() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    Blynk.virtualWrite(V2, t);
    Blynk.virtualWrite(V4, h);
  }
}

// ------------------------------------
// SETUP
// ------------------------------------
void setup() {
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);

  dht.begin();

  pinMode(OVERRIDE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(JOY_SW_PIN, INPUT_PULLUP);

  Blynk.syncVirtual(V0);

  timer.setInterval(100L, readLaserSensor);
  timer.setInterval(1000L, readDHT);
  timer.setInterval(1000L, readFlameSensor);
  timer.setInterval(1000L, readLightSensor);
}

// ------------------------------------
// LOOP
// ------------------------------------
void loop() {
  Blynk.run();
  timer.run();

  readJoystick();
  readJoystickButton();
  handleOverrideButtonPress();

  char key = keypad.getKey();
  if (key) handlePasswordInput(key);
}
