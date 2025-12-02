#include <Keypad.h>

// --- LED PINS ---
#define RED_LED_PIN 8
#define BLUE_LED_PIN 7
#define BUZZER_PIN 12
#define CORRECT_LED 9
#define WRONG_LED 13

// --- ANALOG INPUT PINS ---
#define A1_PIN A1
#define A2_PIN A2
#define A3_PIN A3

// --- Threshold ---
const int ANALOG_THRESHOLD = 143;  // approx for 0.7V
int lastPwm = 0;

bool unlocked = false;

// --- BUZZER VARIABLES ---
unsigned long buzzerTimer = 0;
bool buzzerState = false;
const int buzzerInterval = 200;   // 200 ms blink interval

// --- PASSWORD / KEYPAD ---
const String PASSWORD = "1111";    // your password
String inputCode = "";

// Define keypad layout
const byte ROWS = 3;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'9','6','3'},
  {'4','5','2'},
  {'7','4','1'}
};
byte rowPins[ROWS] = {0, 1, 2};  // D0, D1, D2
byte colPins[COLS] = {3, 4, 5};  // D3, D4, D5

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(9600);
  Serial.println("--- Combined System with Keypad ---");

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(CORRECT_LED, OUTPUT);
  pinMode(WRONG_LED, OUTPUT);
  pinMode(6, OUTPUT);   // PWM pin for A3
}

void loop() {
  // -------------------------
  // READ ANALOG INPUTS A1 / A2
  // -------------------------
  int a1 = analogRead(A1_PIN);
  int a2 = analogRead(A2_PIN);

  bool A1_High = (a1 > ANALOG_THRESHOLD);
  bool A2_High = (a2 > ANALOG_THRESHOLD);

  // -------------------------
  // LED LOGIC
  // -------------------------
  if (A1_High && A2_High) {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(BLUE_LED_PIN, HIGH);
  } else if (A1_High) {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(BLUE_LED_PIN, LOW);
  } else if (A2_High) {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, HIGH);
  } else {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);
  }

  // -------------------------
  // BUZZER ON A0 > 0.7V
  // -------------------------
  int rawValue = analogRead(A0);
  if (rawValue > ANALOG_THRESHOLD) {
    unsigned long now = millis();
    if (now - buzzerTimer >= buzzerInterval) {
      buzzerTimer = now;
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerState = false;
  }

  // -------------------------
  // READ AND PRINT A3 -> PWM6
  // -------------------------
  int a3 = analogRead(A3_PIN);
  int pwmValue = map(a3, 20, 660, 0, 255); // map to 0-255 for PWM
  if(pwmValue < 2) {
    digitalWrite(6, LOW);
  }
  else if(pwmValue > 250) {
    digitalWrite(6, HIGH);
  }
  else if(pwmValue > lastPwm+3 || pwmValue < lastPwm-3){
      analogWrite(6, pwmValue);          // write PWM to pin 6
      lastPwm = pwmValue;
  }

  // -------------------------
  // KEYPAD INPUT
  // -------------------------
  char key = keypad.getKey();
  if (key) {
    inputCode += key;
    Serial.print("Key pressed: ");
    Serial.println(key);

    if (inputCode.length() >= 4) {   // 4 digits entered
      if (inputCode == PASSWORD) {
        unlocked = !unlocked;
        digitalWrite(CORRECT_LED, unlocked);  // correct password
        digitalWrite(WRONG_LED, LOW);
        Serial.println("Password correct!");
      } else {
        digitalWrite(WRONG_LED, HIGH);    // wrong password blink
        digitalWrite(CORRECT_LED, unlocked);
        Serial.println("Password wrong!");
        delay(1000);
        digitalWrite(WRONG_LED, LOW);
        digitalWrite(CORRECT_LED, unlocked);
      }
      inputCode = "";  // reset input
    }
  }
}
