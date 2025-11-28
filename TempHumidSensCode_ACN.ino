#include "DHT.h"

// --- Pin Definitions ---
#define DHTPIN 2      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11 // Or DHT22

#define RED_LED_PIN 10   // Digital Pin for the Red Color
#define BLUE_LED_PIN 11 // Digital Pin for the Green Color

// --- Thresholds ---
// Define the temperature thresholds in Celsius (change these as needed)
const float LOW_TEMP_THRESHOLD = 23.0; // Below this is "Cold" (Green)
const float HIGH_TEMP_THRESHOLD = 25.0; // Above this is "Hot" (Red)

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  // Initialize Serial communication at 9600 baud rate
  Serial.begin(9600);
  Serial.println("--- Temp & Humidity LED Monitor ---");

  // Initialize LED pins as outputs
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  // Start the DHT sensor
  dht.begin();
}

void loop() {
  // Wait 2 seconds between sensor readings
  delay(1000);

  // --- 1. Read Sensor Data ---
  // Reading humidity and temperature takes time!
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Check if any reads failed
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor! Check wiring.");
    return;
  }

  // --- 2. Print Real-Time Data to Serial Monitor ---
  Serial.print("Current Temp: ");
  Serial.print(t);
  Serial.print(" *C | Humidity: ");
  Serial.print(h);
  Serial.print(" % | Status: ");

  // --- 3. LED Color Logic (Based on Temperature only) ---
  if (t < LOW_TEMP_THRESHOLD) {
    // Cold: Turn on Blue
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, HIGH);
    Serial.println("Cold (Blue LED)");

  } else if (t >= HIGH_TEMP_THRESHOLD) {
    // Hot: Turn on RED
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(BLUE_LED_PIN, LOW);
    Serial.println("Hot (Red LED)");

  } else {
    // Moderate/Warm: Turn BOTH ON for Purple
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(BLUE_LED_PIN, HIGH);
    Serial.println("Moderate (Purple LED)");
  }
}