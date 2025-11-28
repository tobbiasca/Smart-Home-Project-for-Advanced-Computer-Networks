#define BLYNK_TEMPLATE_ID "TMPL6gaffk8Oo"
#define BLYNK_TEMPLATE_NAME "Smart Home"
#define BLYNK_AUTH_TOKEN "NBxrtNTVfA_zGSfOGMSS7Qa158AOn3Ag"

#define BLYNK_PRINT Serial

#include <BlynkSimpleWioTerminal.h> 

//char ssid[] = "NTUST-PEAP";
char ssid[] = "iPhone Florencia";
char pass[] = "flgc260803";

// --- 1. Pin Definitions ---
const int RED_PIN = D2;
const int GREEN_PIN = D2;
const int BLUE_PIN = D2;
const int ZONE1_SINGLE_LED_PIN = D8; // for Zone 1 LED

const int LDR_PIN = A6; // LDR Analog Pin 
const int OVERRIDE_BUTTON_PIN = D3; // HW483 Button Signal Pin (ON/OFF Toggle)

// Rotary Encoder Pins
const int ENCODER_CLK_PIN = D7; // Connect to D3 (Interrupt 1)
const int ENCODER_DT_PIN = D5; // Connect to D2 (Interrupt 0)
const int ENCODER_SW_PIN = D4; // Encoder Switch Pin (Zone Cycling)

// --- 2. LDR Calibration Constants ---
const int LDR_MIN = 300; // Analog value for Bright ambient light
const int LDR_MAX = 700; // Analog value for Dark ambient light

// --- 3. State and Control Variables ---
// Brightness Source Mode: 0 = Automatic (LDR) | 1 = Manual (Encoder)
int lightingMode = 1; 
// Button State for Debouncing/Toggling
int lastOverrideButtonState = HIGH; 

// Rotary Encoder variables
volatile int encoderPos = 170; // Tracks the current encoder rotation (0-255)
const int ENCODER_STEP_SIZE = 20; // Change this value (e.g., 5, 10, or 20)
int lastClkState; 

// Zone Management
const int NUM_ZONES = 2; 
int currentZone = 0;

// Array to store the ON/OFF state for each zone. (RESTORED DECLARATION)
int zonePowerState[NUM_ZONES] = {1, 1}; // Start all zones ON (1)

// Store the brightness level (0-255) for each zone
int zoneBrightness[NUM_ZONES] = {255, 255}; 

// --- VIRTUAL PIN DEFINITIONS ---
#define V0_POWER_TOGGLE   V0  // Remote ON/OFF Toggle
#define V1_MODE_TOGGLE    V1  // Remote AUTO/MANUAL Toggle
#define V2_ZONE_DISPLAY   V2  // Zone Selection Display
#define V3_REMOTE_BRIGHT  V3  // Remote Brightness Input / Output Monitor
#define V4_LDR_MONITOR    V4  // LDR Reading Monitoring

// === BLYNK WRITE HANDLERS (Remote Input) ===

// Handler for Remote Power ON/OFF (V0) - Configured as a Switch
BLYNK_WRITE(V0) {
  // V0 sends the actual power state (0 or 1)
  int remotePowerState = param.asInt(); 
  
  // Update the power state for the current zone
  zonePowerState[currentZone] = remotePowerState;
  
  Serial.print("--> Remote Power Set for Zone ");
  Serial.println(currentZone);
}

// Handler for Remote AUTO/MANUAL Brightness Mode (V1) - Configured as a Switch
BLYNK_WRITE(V1) {
    // V1 sends the actual mode state (0 for AUTO, 1 for MANUAL)
    lightingMode = param.asInt();
    // --- Widget Control Logic ---
    if (lightingMode == 0) { // Switched to AUTOMATIC Mode
        // Disable all manual control widgets
        Blynk.setProperty(V0_POWER_TOGGLE, "isDisabled", "true");  // Power ON/OFF
        Blynk.setProperty(V2_ZONE_DISPLAY, "isDisabled", "true");   // Zone Selection Menu
        Blynk.setProperty(V3_REMOTE_BRIGHT, "isDisabled", "true");  // Brightness Slider
        zonePowerState[0] = 1; // Turn ON
        zonePowerState[1] = 1; // Turn ON
        Serial.println("--> Switched to AUTO: Manual controls disabled.");
    } else { // Switched to MANUAL Mode
        // Enable all manual control widgets
        Blynk.setProperty(V0_POWER_TOGGLE, "isDisabled", "false"); // Power ON/OFF
        Blynk.setProperty(V2_ZONE_DISPLAY, "isDisabled", "false");  // Zone Selection Menu
        Blynk.setProperty(V3_REMOTE_BRIGHT, "isDisabled", "false"); // Brightness Slider
        
        Serial.println("--> Switched to MANUAL: Manual controls enabled.");
    }
}

// Handler for Remote Zone Selection (V2) - Configured as a Menu Widget (NEW!)
BLYNK_WRITE(V2) {
  // Menu widgets send a 1-based index (1, 2, 3...)
  int remoteZoneIndex = param.asInt();
  
  // Convert 1-based index to 0-based array index
  int newZone = remoteZoneIndex; 

  if (newZone >= 0 && newZone < NUM_ZONES) {
      
      // 1. Save the current brightness state before leaving the zone
      noInterrupts();
      zoneBrightness[currentZone] = encoderPos; 
      interrupts();

      // 2. Update the current zone
      currentZone = newZone; 

      // 3. Load the new zone's brightness and sync the encoder
      encoderPos = zoneBrightness[currentZone];
      
      Serial.print("--> Remote Zone Selected: ");
      Serial.println(currentZone);
  }
}

// Handler for Remote Manual Brightness Slider (V3)
BLYNK_WRITE(V3) {
  int remoteBrightness = param.asInt(); 
  
  if (lightingMode == 1) {
    zoneBrightness[currentZone] = remoteBrightness; 
    
    // Sync the local encoder position to the remote value
    noInterrupts();
    encoderPos = remoteBrightness; 
    interrupts();
    
    Serial.print("--> Remote Brightness Set to: ");
    Serial.println(remoteBrightness);
  }
}


// === INTERRUPT SERVICE ROUTINE (ISR) FOR ENCODER ROTATION ===
void updateEncoder() {
  int currentClkState = digitalRead(ENCODER_CLK_PIN);
  
  if (currentClkState != lastClkState) {
    if (digitalRead(ENCODER_DT_PIN) != currentClkState) {
      //encoderPos++; // Clockwise
      encoderPos += ENCODER_STEP_SIZE;
    } else {
      //encoderPos--; // Counter-Clockwise
      encoderPos -= ENCODER_STEP_SIZE;
    }
    
    encoderPos = constrain(encoderPos, 0, 255);
    
    if (lightingMode == 1) {
      zoneBrightness[currentZone] = encoderPos;
    }
  }
  lastClkState = currentClkState;
}


// Function to handle the Rotary Encoder Switch (SW) press for Zone Cycling
void handleZoneSwitch() {
  int swState = digitalRead(ENCODER_SW_PIN);
  
  if (swState == LOW) {
    delay(100); 
    
    if (digitalRead(ENCODER_SW_PIN) == LOW) {
      
      // Save the current brightness level before switching
      noInterrupts();
      zoneBrightness[currentZone] = encoderPos; 
      interrupts();
      
      // Cycle to the next zone
      currentZone = (currentZone + 1) % NUM_ZONES;
      
      // Load the brightness level of the new zone and sync encoder position
      encoderPos = zoneBrightness[currentZone]; 

      // Update Blynk Menu Widget V2 (1-based index)
      Blynk.virtualWrite(V2_ZONE_DISPLAY, currentZone);
      
      Serial.print("Zone Switched to: ");
      Serial.println(currentZone);
      
      while(digitalRead(ENCODER_SW_PIN) == LOW);
    }
  }
}

// Function to handle the HW483 button press (ON/OFF Toggle)
void handleOverrideButtonPress() {
  int currentButtonState = digitalRead(OVERRIDE_BUTTON_PIN);

  if (currentButtonState != lastOverrideButtonState) {
    delay(50); 
    
    if (digitalRead(OVERRIDE_BUTTON_PIN) == LOW) {
      // Toggle the powerState FOR THE CURRENT ZONE
      if (zonePowerState[currentZone] == 0) {
        zonePowerState[currentZone] = 1; // Turn ON
        //Serial.print("--> Zone "); Serial.print(currentZone); Serial.println(" Power ON.");
      } else {
        zonePowerState[currentZone] = 0; // Turn OFF
        //Serial.print("--> Zone "); Serial.print(currentZone); Serial.println(" Power OFF.");
      }
      Serial.print("--> Zone "); Serial.print(currentZone); Serial.println(" Local Power Toggled.");
    }
  }
  lastOverrideButtonState = currentButtonState;
}

// Function to set the RGB LED color and brightness (SIMPLIFIED)
void setLEDColor(int brightness) {
  int value = constrain(brightness, 0, 255);

  if(currentZone == 0 && lightingMode == 1){
    // Zone 0 -> RGB Led
    analogWrite(RED_PIN, value);
    analogWrite(GREEN_PIN, value);
    analogWrite(BLUE_PIN, value);
  } else if(currentZone == 1 && lightingMode == 1) {
    // Zone 1 -> Single Led
    analogWrite(ZONE1_SINGLE_LED_PIN, value);
  }else{
    // lightningMode == 0 (AUTO) should be for all zones
    analogWrite(RED_PIN, value);
    analogWrite(GREEN_PIN, value);
    analogWrite(BLUE_PIN, value);
    analogWrite(ZONE1_SINGLE_LED_PIN, value);
  }

}


void setup() {
  // Set output pins for RGB LED
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(ZONE1_SINGLE_LED_PIN, OUTPUT); // zone 1 pin
  
  // Set input pins with internal pull-up resistors
  pinMode(OVERRIDE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
  pinMode(ENCODER_DT_PIN, INPUT_PULLUP);
  pinMode(ENCODER_SW_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT_PIN), updateEncoder, CHANGE); 

  Serial.begin(9600);

  // Blynk connection
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass); 
  Serial.println("Blynk Connected. Waiting for Ready..."); 
  
  // Set the initial state of the V2 Menu Widget to Zone 1 (index 1)
  Blynk.virtualWrite(V2_ZONE_DISPLAY, 0);
  
  Serial.println("Smart Lighting System Initialized.");
}

void loop() {
  Blynk.run();

  handleOverrideButtonPress(); // Checks HW483 button (ON/OFF)
  handleZoneSwitch();          // Rotary SW (Zone Cycling)

  int finalBrightness = 0; 
  int ldrValue = 0; // Value must be initialized here

  // --- 1. DETERMINE BRIGHTNESS SOURCE ---
  if (lightingMode == 0) {
    // AUTOMATIC BRIGHTNESS CONTROL (LDR)
    ldrValue = analogRead(LDR_PIN);
    finalBrightness = map(ldrValue, LDR_MIN, LDR_MAX, 0, 255);
    finalBrightness = constrain(finalBrightness, 0, 255);
  } else {
    // MANUAL BRIGHTNESS CONTROL (Rotary Encoder)
    finalBrightness = zoneBrightness[currentZone]; 
  }
  
  // --- 2. APPLY POWER STATE ---
  if (zonePowerState[currentZone] == 0) {
    finalBrightness = 0; 
  }

  setLEDColor(finalBrightness);

  // --- Blynk Synchronization (Output) ---
  Blynk.virtualWrite(V0_POWER_TOGGLE, zonePowerState[currentZone]); 
  Blynk.virtualWrite(V1_MODE_TOGGLE, lightingMode);              
  Blynk.virtualWrite(V3_REMOTE_BRIGHT, finalBrightness);         
  Blynk.virtualWrite(V4_LDR_MONITOR, ldrValue);

  // --- 3. DEBUG OUTPUT (CRITICAL PROTECTED SECTION) ---
  
  // Create status strings before the protected block
  String modeStr = lightingMode == 0 ? "AUTO" : "MANUAL";
  
  // Protect ALL Serial I/O
  noInterrupts(); 
  
  Serial.print("Mode: ");
  Serial.print(modeStr);
  Serial.print(" | Power: ");
  Serial.print(zonePowerState[currentZone] ? "ON" : "OFF"); // Use zonePowerState
  Serial.print(" | Zone: ");
  Serial.print(currentZone);
  Serial.print(" | Brightness: ");
  Serial.print(finalBrightness);
  
  if (lightingMode == 0) {
    Serial.print(" | LDR: ");
    // Print the LDR value read earlier in the loop
    Serial.print(ldrValue);
  }
  Serial.println();

  interrupts(); 

  delay(200); 
}