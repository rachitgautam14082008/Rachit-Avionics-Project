/*
 * SEDS BPHC - AVIONICS INDUCTION TASK 2
 * Odysseus Navigation State Machine
 * 
 * Hardware Pins:
 * - Push Button: Pin 2 (Internal Pullup)
 * - Ultrasonic Sensor: Trig Pin 9, Echo Pin 8
 * - Piezo Buzzer: Pin 11
 * - Warning LED: Pin 13
 * - Photoresistor (LDR): Pin A0
 * - 16x2 LCD: Pins 7, 6, 5, 4, 3, 10
 */

#include <LiquidCrystal.h>

// Initialize LCD library with pin assignments (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(7, 6, 5, 4, 3, 10);

// Hardware Pin Definitions
const int TRIG_PIN   = 9;
const int ECHO_PIN   = 8;
const int LDR_PIN    = A0;
const int BUTTON_PIN = 2;
const int BUZZER_PIN = 11;
const int LED_PIN    = 13;

// Threshold values for triggering hazards
const int DISTANCE_THRESHOLD = 100; // Trigger Charybdis if closer than 100cm
const int LIGHT_THRESHOLD    = 512; // Trigger Storm threshold for LDR

// State Definitions: 0=OPEN_SEA, 1=STORM, 2=CHARYBDIS, 3=ANCHOR_DROPPED, 4=WRECKED
int currentState = 0;

// Timing and Tracking Variables
unsigned long startTime     = 0; // Tracks when a 5-second hazard timer begins
unsigned long lastBlinkTime = 0; // Tracks the 200ms LED blinking intervals
unsigned long lastPress     = 0; // Debounce timer for the anchor button
int ledState                = LOW;  // Keeps track of LED being ON or OFF
bool anchorActive           = false; // Remembers if anchor is currently toggled ON
int lastButtonState         = HIGH; // Keeps track of previous button state

void setup() {
  // Set up pin modes
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Uses internal resistor (LOW = Pressed)
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Initialize 16x2 LCD display
  lcd.begin(16, 2);
  lcd.print("ODYSSEUS NAV");
  lcd.setCursor(0, 1);
  lcd.print("System Ready");
  delay(1000);
  lcd.clear();
}

void loop() {
  // --- 1. SENSOR READINGS ---

  // Read Light Sensor (LDR)
  int lightVal = analogRead(LDR_PIN);

  // Read Ultrasonic Sensor Distance in cm
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 20000);
  int distVal = (duration == 0) ? 400 : (duration * 0.0343 / 2);

  // Button Toggle Logic (Toggles anchor state ON/OFF when button is clicked)
  int currentButton = digitalRead(BUTTON_PIN);
  if (currentButton == LOW && lastButtonState == HIGH && (millis() - lastPress > 250)) {
    anchorActive = !anchorActive; // Toggle state
    lastPress = millis();
  }
  lastButtonState = currentButton;

  // Determine hazard conditions from sensor inputs
  bool stormCondition     = (lightVal < 400 || lightVal > 600); // LDR movement detected
  bool charybdisCondition = (distVal > 0 && distVal < DISTANCE_THRESHOLD);

  // --- 2. STATE MACHINE LOGIC ---

  // Priority 1: Once WRECKED (4), nothing can override it until restart
  if (currentState != 4) {

    // Priority 2: ANCHOR OVERRIDE (3) takes immediate precedence over everything else
    if (anchorActive) {
      currentState = 3;
      startTime = 0; // Reset hazard timer
    }
    // Return to Open Sea if anchor is un-toggled while in safe state
    else if (currentState == 3 && !anchorActive) {
      currentState = 0;
    }
    // Priority 3: LATCHED HAZARD STATES
    // If in STORM, stay in storm until hazard clears or 5s runs out
    else if (currentState == 1) {
      if (!stormCondition) {
        currentState = 0;
        startTime = 0;
      }
    }
    // If in CHARYBDIS, stay until clear or 5s runs out
    else if (currentState == 2) {
      if (!charybdisCondition) {
        currentState = 0;
        startTime = 0;
      }
    }
    // DEFAULT: OPEN SEA (0) - Check for new incoming hazards
    else {
      if (stormCondition) {
        currentState = 1;
        startTime = millis(); // Lock start time for 5s countdown
      } 
      else if (charybdisCondition) {
        currentState = 2;
        startTime = millis(); // Lock start time for 5s countdown
      }
    }

// 5-SECOND COUNTDOWN CHECK: If in hazard for >= 5s, ship becomes WRECKED
    if ((currentState == 1 || currentState == 2) && startTime > 0) {
      if (millis() - startTime >= 5000) {
        currentState = 4; // WRECKED
      }
    }
  }

  // --- 3. DISPLAY & HARDWARE OUTPUT CONTROL ---

  lcd.setCursor(0, 0);

  // STATE 0: OPEN SEA
  if (currentState == 0) {
    lcd.print("STATE: OPEN SEA ");
    lcd.setCursor(0, 1);
    lcd.print("Status: Normal  ");
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
  }
  // STATE 1: STORM
  else if (currentState == 1) {
    lcd.print("STATE: STORM    ");
    lcd.setCursor(0, 1);
    lcd.print("Timer: ");
    lcd.print(5 - (millis() - startTime) / 1000); // Print remaining seconds
    lcd.print("s left  ");

    noTone(BUZZER_PIN); // No sound in storm
    
    // Non-blocking 200ms LED blink logic
    if (millis() - lastBlinkTime >= 200) {
      lastBlinkTime = millis();
      ledState = (ledState == LOW) ? HIGH : LOW;
      digitalWrite(LED_PIN, ledState);
    }
  }
  // STATE 2: CHARYBDIS
  else if (currentState == 2) {
    lcd.print("STATE: CHARYBDIS");
    lcd.setCursor(0, 1);
    lcd.print("Timer: ");
    lcd.print(5 - (millis() - startTime) / 1000); // Print remaining seconds
    lcd.print("s left  ");

    digitalWrite(LED_PIN, LOW); // No LED in Charybdis
    tone(BUZZER_PIN, 800);      // Continuous 800 Hz alarm tone
  }
  // STATE 3: ANCHOR DROPPED
  else if (currentState == 3) {
    lcd.print("STATE: ANCHORED ");
    lcd.setCursor(0, 1);
    lcd.print("Status: Safe    ");
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
  }
  // STATE 4: WRECKED
  else if (currentState == 4) {
    lcd.print("STATE: WRECKED  ");
    lcd.setCursor(0, 1);
    lcd.print("SHIP DESTROYED! ");
    digitalWrite(LED_PIN, HIGH); // Solid LED ON
    tone(BUZZER_PIN, 200);       // Continuous low 200 Hz tone
  }

  delay(30); // Short delay for loop stability
}