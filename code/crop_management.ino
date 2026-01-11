#include <Adafruit_LiquidCrystal.h>  // Include Adafruit's LCD library
#include <stdlib.h>                   // For memory allocation functions

// Pin Definitions
const int switchPin = 2;           // Pin connected to the mode switch
const int led1Pin = 12;            // Green LED for automatic mode
const int led2Pin = 13;            // Red LED for manual mode
const int moistureSensorPin = A0;  // Soil moisture sensor analog pin
const int tempSensorPin = A1;      // Temperature sensor analog pin
const int redPin = 11;             // Red pin of RGB LED
const int greenPin = 10;           // Green pin of RGB LED
const int bluePin = 9;             // Blue pin of RGB LED
const int outputBluePin = 8;       // Output Blue LED in automatic mode
const int buzzerPin = 7;           // Buzzer for manual mode


// Initialize the LCD
Adafruit_LiquidCrystal lcd_1(0x27); // Adjust to your I2C address

// Enum to represent system mode
enum Mode {
  AUTOMATIC,
  MANUAL
};

// Structures for sensor data and system state
struct SensorData {
  int moistureValue;
  int tempValue;
  float temperatureC;
};

struct SystemState {
  Mode currentMode;
  int rgbState;
};

// Create instances of structures
SensorData sensorData;
SystemState systemState;

// Create pointers to structures
SensorData* sensorDataPtr = &sensorData;
SystemState* systemStatePtr = &systemState;

// Threshold Values
const int lowMoistureThreshold = 300;   // Adjust based on your sensor
const int mediumMoistureThreshold = 600; // Adjust based on your sensor
const float mediumTemperatureThreshold = 25.0; // Temperature threshold in °C

// Timing Variables
unsigned long lastSensorReadTime = 0;   // Last time sensors were read
unsigned long lastChangeTime = 0;       // Last time a change was detected
const unsigned long sensorReadInterval = 1000;  // Read sensors every 1 second

// Array to store time values for changes
unsigned long* timeArray = NULL;
int timeArraySize = 0;  // Track the current size of the array

// Last recorded values for comparison
int lastMoistureValue = 0;
float lastTemperatureC = 0;

void setup() {
  Serial.begin(9600);                  // Initialize Serial Monitor for printing
  lcd_1.begin(16, 2);                  // Initialize the LCD
  lcd_1.setBacklight(1);               // Turn on the backlight

  // Initialize pins
  pinMode(switchPin, INPUT_PULLUP);    // Internal pull-up for the mode switch
  pinMode(led1Pin, OUTPUT);            // Green LED pin (automatic mode)
  pinMode(led2Pin, OUTPUT);            // Red LED pin (manual mode)
  pinMode(redPin, OUTPUT);             // Set RGB pins as output
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(outputBluePin, OUTPUT);      // Blue LED output in automatic mode
  pinMode(buzzerPin, OUTPUT);          // Buzzer pin in manual mode
}

void loop() {
  unsigned long currentTime = millis(); // Get the current time

  updateSystemMode();                   // Read and update system mode

  // Check if it's time to read sensors
  if (currentTime - lastSensorReadTime >= sensorReadInterval) {
    readSensorValues();                 // Read sensor values
    lastSensorReadTime = currentTime;   // Update last read time
  }

  // Check for changes in moisture or temperature
  if (hasSensorValueChanged()) {
    lastChangeTime = currentTime;       // Update the last change time
    storeTimeValue(lastChangeTime / 1000); // Store the change time in the array
    displaySensorData(lastChangeTime / 1000); // Display values with time on LCD (in seconds)
  }

  // Determine RGB color based on conditions
  updateRGBState();

  // Control LEDs and Buzzer based on system mode
  controlAutomaticMode();
  controlManualMode();
}

// Function to read sensor values
void readSensorValues() {
  sensorDataPtr->moistureValue = analogRead(moistureSensorPin);  // Read soil moisture
  sensorDataPtr->tempValue = analogRead(tempSensorPin);          // Read temperature
  sensorDataPtr->temperatureC = (sensorDataPtr->tempValue - 113) / 2.04; // Convert to Celsius
}

// Function to check if there was a change in moisture or temperature values
bool hasSensorValueChanged() {
  bool changed = false;

  if (sensorDataPtr->moistureValue != lastMoistureValue) {
    lastMoistureValue = sensorDataPtr->moistureValue;
    changed = true;
  }

  if (sensorDataPtr->temperatureC != lastTemperatureC) {
    lastTemperatureC = sensorDataPtr->temperatureC;
    changed = true;
  }

  return changed;
}

// Function to store the time value in the dynamic array and print the array contents
void storeTimeValue(unsigned long time) {
  timeArraySize++;  // Increase the size for the new element
  timeArray = (unsigned long*) realloc(timeArray, timeArraySize * sizeof(unsigned long));
  
  if (timeArray != NULL) {
    timeArray[timeArraySize - 1] = time; // Store the new time value in the last position
    
    // Print the entire array to the Serial Monitor
    Serial.println("Updated timeArray contents:");
    for (int i = 0; i < timeArraySize; i++) {
      Serial.print("Time [");
      Serial.print(i);
      Serial.print("]: ");
      Serial.println(timeArray[i]);
    }
  }
}

// Function to update system mode based on switch state
void updateSystemMode() {
  int switchState = digitalRead(switchPin);  // Read mode switch state
  if (switchState == LOW) {  // Switch is ON (automatic mode)
    systemStatePtr->currentMode = AUTOMATIC;
  } else {  // Switch is OFF (manual mode)
    systemStatePtr->currentMode = MANUAL;
  }

  // Control LEDs based on system mode
  if (systemStatePtr->currentMode == AUTOMATIC) {
    digitalWrite(led1Pin, HIGH);  // Turn on Green LED for automatic mode
    digitalWrite(led2Pin, LOW);   // Turn off Red LED
  } else {
    digitalWrite(led1Pin, LOW);   // Turn off Green LED
    digitalWrite(led2Pin, HIGH);  // Turn on Red LED for manual mode
  }
}

// Function to display sensor data on the LCD
void displaySensorData(unsigned long seconds) {
  lcd_1.clear();
  lcd_1.setCursor(0, 0);
  lcd_1.print("Moisture: ");
  lcd_1.print(sensorDataPtr->moistureValue);
  
  lcd_1.setCursor(0, 1);
  lcd_1.print("Temp: ");
  lcd_1.print(sensorDataPtr->temperatureC);
  
  // Display the time in seconds of the last change
  lcd_1.setCursor(8, 1);
  lcd_1.print("Time: ");
  lcd_1.print(seconds);
  lcd_1.print("s");
}

// Function to update RGB state based on moisture and temperature
void updateRGBState() {
  if (sensorDataPtr->moistureValue <= lowMoistureThreshold) {
    // Red: Low moisture
    setRGBColor(255, 0, 0);  // Red
    systemStatePtr->rgbState = 2;
  } else if (sensorDataPtr->moistureValue < mediumMoistureThreshold && sensorDataPtr->temperatureC > mediumTemperatureThreshold) {
    // Yellow: Medium moisture and high temperature
    setRGBColor(255, 180, 0); // PINK
    systemStatePtr->rgbState = 1;
  } else {
    // Green: Sufficient moisture
    setRGBColor(0, 255, 0);  // BLUE
    systemStatePtr->rgbState = 0;
  }
}

// Function to control automatic mode behavior
void controlAutomaticMode() {
  if (systemStatePtr->currentMode == AUTOMATIC) {
    digitalWrite(buzzerPin, LOW); // Ensure buzzer is OFF
    if (systemStatePtr->rgbState == 2) {  // Red
      analogWrite(outputBluePin, 255);  // Full brightness
    } else if (systemStatePtr->rgbState == 1) {  // Yellow
      analogWrite(outputBluePin, 128);  // Half brightness
    } else if (systemStatePtr->rgbState == 0) {  // Green
      digitalWrite(outputBluePin, LOW); // Off
    }
  }
}

// Function to control manual mode behavior
void controlManualMode() {

  if (systemStatePtr->currentMode == MANUAL) {
    if (sensorDataPtr->moistureValue > lowMoistureThreshold && sensorDataPtr->moistureValue < mediumMoistureThreshold && sensorDataPtr->temperatureC > mediumTemperatureThreshold) {
      // Case for medium moisture and high temperature
      digitalWrite(buzzerPin, HIGH); // Turn on buzzer
      setRGBColor(255, 180, 0);      // Yellow RGB color
    } else if (sensorDataPtr->moistureValue > lowMoistureThreshold) {
      // Sufficient moisture, buzzer off
      digitalWrite(buzzerPin, LOW);
      setRGBColor(0, 255, 0);        // Green RGB color
    } else {
      // Low moisture, buzzer on
      digitalWrite(buzzerPin, HIGH);
      setRGBColor(255, 0, 0);        // Red RGB color
    }

  }
}

// Helper function to set RGB LED color
void setRGBColor(int r, int g, int b) {
  analogWrite(redPin, r);
  analogWrite(greenPin, g);
  analogWrite(bluePin, b);
}