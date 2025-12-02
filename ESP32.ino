#include <DHT.h>
#include <WiFi.h>

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL27PezZL51"
#define BLYNK_TEMPLATE_NAME "Code"

#include <BlynkSimpleEsp32.h>

// --------------------
// WIFI & BLYNK
// --------------------
char ssid[] = "NIRI";
char pass[] = "123454321";
char auth[] = "17OZLEEf8nnmzWdaWwVZFvu7uE9Zdyas";

BlynkTimer timer;

// --------------------
// AC CONTROL VARIABLES
// --------------------
int autoMode = 0;   // V10
int acState = 0;    // V7
int tempMode = 0;   // V9

float COLD_THRESHOLD = 23.0;
float HOT_THRESHOLD  = 27.0;

#define AC_COLD_PIN 22
#define AC_HOT_PIN 21
#define LOCK 14
#define JOY_DAC_PIN 25  // now using DAC1

// --------------------
// DHT SENSOR
// --------------------
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --------------------
// ANALOG SENSORS
// --------------------
const int lightPin = 35;
const int flamePin = 32;
const int laserPin = 33;

// --------------------
// JOYSTICK
// --------------------
#define JOY_Y_PIN 36
int joystickValue = 128;
int lastJoySent = -1;

const int JOY_STEP = 20;        
const int JOY_DEADZONE_UP = 3500;
const int JOY_DEADZONE_DOWN = 800;
const unsigned long JOY_INTERVAL = 80;
unsigned long lastJoyMove = 0;

// --------------------
// GLOBAL AUTO MODE FLAG
// --------------------
int V10_value = 0;

// --------------------
// FUNCTIONS
// --------------------
void updateACpins() {
    if (acState == 0) {
        digitalWrite(AC_COLD_PIN, LOW);
        digitalWrite(AC_HOT_PIN, LOW);
        return;
    }

    if (tempMode == 1) {          // Cold
        digitalWrite(AC_COLD_PIN, HIGH);
        digitalWrite(AC_HOT_PIN, LOW);
    } 
    else if (tempMode == 2) {     // Warm
        digitalWrite(AC_COLD_PIN, HIGH);
        digitalWrite(AC_HOT_PIN, HIGH);
    } 
    else if (tempMode == 3) {     // Hot
        digitalWrite(AC_COLD_PIN, LOW);
        digitalWrite(AC_HOT_PIN, HIGH);
    }
}

void updateTempMode(int mode) {
    tempMode = mode;
    updateACpins();
}

void updateJoystickOutput() {
    // write joystick value (0-255) to DAC pin 25
    dacWrite(JOY_DAC_PIN, joystickValue);
}


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
            updateJoystickOutput(); // update DAC25
        }

        lastJoyMove = now;
    }
}


void readLightSensor() { 
    Blynk.virtualWrite(V3, analogRead(lightPin)); 
}

void readLaserSensor() { 
    Blynk.virtualWrite(V8, analogRead(laserPin) < 20 ? 1 : 0); 
}

void readFlameSensor() { 
    int value = analogRead(flamePin);
    Blynk.virtualWrite(V1, value);
    if (value < 150) {Blynk.virtualWrite(V6, 1), digitalWrite(23, 1);}
}

void readPinD34() {
  int state = digitalRead(LOCK);
  Blynk.virtualWrite(V0, state);
}

void readDHT() {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
        Serial.println("Failed to read DHT!");
        return;
    }

    // Send live readings
    Blynk.virtualWrite(V2, t);
    Blynk.virtualWrite(V4, h);

    // Auto mode: update tempMode and AC pins
    if (V10_value == 1) {
        int mode;
        if (t < COLD_THRESHOLD) mode = 1;
        else if (t >= COLD_THRESHOLD && t < HOT_THRESHOLD) mode = 2;
        else mode = 3;

        updateTempMode(mode);
        Blynk.virtualWrite(V9, mode);  // update app
        Serial.print("Auto temp mode (V9) updated to: ");
        Serial.println(mode);
    }
}

// --------------------
// BLYNK HANDLERS
// --------------------
BLYNK_WRITE(V5) { // joystick updated from app
    joystickValue = param.asInt();
    Serial.print("Joystick (V5) updated from Blynk: ");
    Serial.println(joystickValue);
    updateJoystickOutput(); // write to DAC25
    lastJoySent = joystickValue;
}

BLYNK_WRITE(V10) {
    V10_value = param.asInt();
    autoMode = V10_value;
    Serial.print("Auto mode (V10): ");
    Serial.println(autoMode);
}

BLYNK_WRITE(V7) {
    acState = param.asInt();
    Serial.print("AC state (V7): ");
    Serial.println(acState);
    updateACpins();
}

BLYNK_WRITE(V9) {
    tempMode = param.asInt();
    Serial.print("Temp mode (V9): ");
    Serial.println(tempMode);
    updateACpins();
}

BLYNK_WRITE(V6) {
    int pinValue = param.asInt();
    Serial.print("V6 changed to: ");
    Serial.println(pinValue);
    digitalWrite(23, pinValue);
}

// --------------------
// SETUP
// --------------------
void setup() {
    Serial.begin(115200);
    Blynk.begin(auth, ssid, pass);

    pinMode(23, OUTPUT);
    pinMode(AC_COLD_PIN, OUTPUT);
    pinMode(AC_HOT_PIN, OUTPUT);

    dht.begin();

    timer.setInterval(100L, readLaserSensor);
    timer.setInterval(100L, readDHT);
    timer.setInterval(100L, readFlameSensor);
    timer.setInterval(100L, readLightSensor);
    timer.setInterval(100L, readPinD34);

    // initialize DAC output
    dacWrite(JOY_DAC_PIN, joystickValue);
}


// --------------------
// LOOP
// --------------------
void loop() {
    Blynk.run();
    timer.run();
    readJoystick();
}
