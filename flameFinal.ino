// ESP32 + Flame Sensor + Active Buzzer + Blynk IoT

#define BLYNK_TEMPLATE_ID "TMPL6C3zYVNwf"
#define BLYNK_TEMPLATE_NAME "Fire Alarm"
#define BLYNK_AUTH_TOKEN "K--PCZWrcJ3EhRvGWThgoL33xLFJSI2D"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// WIFI SETTINGS
char ssid[] = "Veronica’s iPhone";
char pass[] = "verovero";

// PINS
#define FLAME_PIN  34      // Flame sensor AO 
#define BUZZER_PIN 25      // Active buzzer + 

bool alarmActive = false;  // if the alarm is activated or not
int flameThreshold = 2000; // threshold for flame

unsigned long buzzerTimer = 0;
bool buzzerState = false;
const int buzzerInterval = 200;   // 200 ms interval

// BLYNK V0 (stop alarm button)
BLYNK_WRITE(V0) {
  static int lastBtn = 0;
  int now = param.asInt();   // 0 or 1

  if (now == 1 && lastBtn == 0) {
    alarmActive = false;
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Alarm stopped from Blynk app.");
  }

  lastBtn = now;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nESP32 BOOTED");

  pinMode(FLAME_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.print("Connecting WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  Blynk.config(BLYNK_AUTH_TOKEN);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {  //if wifi not connected
    static unsigned long t = 0;
    if (millis() - t > 1000) {
      Serial.println("WiFi NOT connected...");
      t = millis();
    }
    return;  //if wifi's not ready, won't go on with the code
  }

  if (!Blynk.connected()) { //if blynk not connected
    static unsigned long b = 0;
    if (millis() - b > 1000) {
      Serial.println("Blynk NOT connected...");
      b = millis();
    }
    Blynk.run(); // try to connect
    return;
  }

  //everything connected properly
  static bool once = false;
  if (!once) {
    Serial.print("WiFi OK, IP = ");
    Serial.println(WiFi.localIP());
    Serial.println("Blynk Connected!");
    once = true;
  }

  Blynk.run(); // working properly

  // read flame
  int v = analogRead(FLAME_PIN);
  Serial.print("Flame = ");
  Serial.println(v);

  // detects flame >> activates alarm
  if (v < flameThreshold && !alarmActive) {
    alarmActive = true;
    Serial.println("FIRE DETECTED! Alarm ON!");
    Blynk.logEvent("fire_alarm");
  }

  // controlling buzzer
  if (alarmActive) {
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


  delay(300);
}
