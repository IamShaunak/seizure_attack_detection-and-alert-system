#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

const uint16_t BUFFER_LENGTH = 100;
uint32_t irBuffer[BUFFER_LENGTH]; // Infrared LED sensor data
uint32_t redBuffer[BUFFER_LENGTH]; // Red LED sensor data
int32_t spo2; // SPO2 value
int8_t validSPO2; // Indicator for valid SPO2 calculation
int32_t heartRate; // Heart rate value
int8_t validHeartRate; // Indicator for valid heart rate calculation

void setup() {
  Serial.begin(115200);

  // Initialize I2C communication
  Wire.begin(21, 22);

  // Initialize the sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    while (1);
  }

  // Set sensor settings
  particleSensor.setup();

  Serial.println(F("MAX30105 initialized. Place your finger on the sensor."));

  delay(1000);
}

void loop() {

  calculate();
  
}

void calculate(){
  // Read sensor data
  for (uint16_t i = 0; i < BUFFER_LENGTH; i++) {
    while (!particleSensor.available()) {
      particleSensor.check();
    }

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();

//    Serial.print(F("Red: "));
//    Serial.print(redBuffer[i]);
//    Serial.print(F(", IR: "));
//    Serial.println(irBuffer[i]);
  }

  // Calculate heart rate and SpO2
  maxim_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_LENGTH, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  Serial.print(F("Heart Rate: "));
  Serial.print(heartRate);
  Serial.print(F(" bpm, SpO2: "));
  Serial.print(spo2);
  Serial.println(F(" %"));

  delay(1000);
}
