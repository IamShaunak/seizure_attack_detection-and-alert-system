
#include <Wire.h>
#include <MPU6050_tockn.h>
#include "MAX30105.h"
#include "WiFi.h"
#include "ThingSpeak.h"
WiFiClient client;
unsigned long lastTime = 0;
unsigned long timerDelay = 300;

//flex sensor
const int FLEX_PIN = 2;
const int THRESHOLD_LOW = 1450;
const int THRESHOLD_HIGH = 1550;
const int FLUCTUATION_TIME = 10; // time in seconds to check for fluctuations
const int FLUCTUATION_THRESHOLD = 60; // percentage of values that must be in fluctuation
const int MIN_READINGS = 15; // minimum number of readings required to trigger seizure detection
const int SAMPLE_RATE = 100; 
bool flexseizureDetected = false; // flag to indicate seizure detection
unsigned long seizureStartTime; // time seizure was first detected
int flexValues[FLUCTUATION_TIME * 1000 / SAMPLE_RATE]; // array to store flex sensor values

//MPU
MPU6050 mpu6050(Wire);
bool flexseizureDetectedFlag = false;
bool mpuseizureDetectedFlag = false;
unsigned long seizureTimer = 0;
const unsigned long seizureTimeThreshold = 5000; // 5 seconds
float accX, accY, accZ;
float prevAccX, prevAccY, prevAccZ;
float accMag, prevAccMag;
float deltaAccMag;
float maxAccMag, minAccMag;
float threshold = 0.04; // adjust this value to set the sensitivity of the seizure detection
unsigned long prevTime, currTime, onsetTime;
int seizureCount = 0;
boolean seizureDetected = false;
boolean onsetDetected = false;
int onsetEventCount = 0;
int detectionDuration = 10000; // detection duration in ms
int onsetThresholdCount = 5; // number of onset events required to trigger a seizure detection
int detectionTimer = 0;

//Wifi
const char* ssid = "sadguru Hostel _EXT";  // Replace with your network SSID
const char* password = "sadguru@1"; 
//const char* ssid = "vedh";  // Replace with your network SSID
//const char* password = "vedh1234"; 
//const char* ssid = "Redmi Note 10S";  // Replace with your network SSID
//const char* password = "kisikonahimilega";  // Replace with your network password
const char* api_key = "K75F2IJ7KTVT8YD1";  // Replace with your ThingSpeak API key
const unsigned long channel_id = 2111814;  // Replace with your ThingSpeak channel ID

void setup() {
  Serial.begin(9600);
  Wire.begin(21, 22);
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);
//  WiFi.mode(WIFI_STA);
  while (!Serial) continue;
   // Connect or reconnect to WiFi
//    if(WiFi.status() != WL_CONNECTED){
//      Serial.print("Attempting to connect");
//      while(WiFi.status() != WL_CONNECTED){
//        WiFi.begin(ssid, password); 
//        delay(5000);     
//      } 
//      Serial.println("\nConnected.");
//    }
  ThingSpeak.begin(client);
  delay(2000);
}

bool flexdetectSeizure() {
  // read flex sensor value
  int flexValue = analogRead(FLEX_PIN);
  Serial.println(flexValue);
  // add flex value to array
  for (int i = 0; i < sizeof(flexValues) / sizeof(flexValues[0]) - 1; i++) {
    flexValues[i] = flexValues[i+1];
  }
  flexValues[sizeof(flexValues) / sizeof(flexValues[0]) - 1] = flexValue;

  // check if flex values are fluctuating rapidly
  int lowCount = 0;
  int highCount = 0;
  for (int i = 0; i < sizeof(flexValues) / sizeof(flexValues[0]); i++) {
    if (flexValues[i] < THRESHOLD_LOW) {
      lowCount++;
    } else if (flexValues[i] > THRESHOLD_HIGH) {
      highCount++;
    }
  }
  float lowPercent = (float)lowCount / sizeof(flexValues) * 100;
  float highPercent = (float)highCount / sizeof(flexValues) * 100;
//  Serial.print("Low count: ");
//  Serial.print(lowCount);
//  Serial.print(", high count: ");
//  Serial.print(highCount);
//  Serial.print(", low percent: ");
//  Serial.print(lowPercent);
//  Serial.print(", high percent: ");
//  Serial.println(highPercent);

  if (lowCount >= MIN_READINGS && highCount >= MIN_READINGS && lowPercent <= 50 && lowPercent >= 10 && highPercent >= 10 && highPercent <= 50) {
    // flex values are fluctuating rapidly, seizure detected
    if (!flexseizureDetected) {
      flexseizureDetected = true;
      seizureStartTime = millis();
      Serial.println("Seizure detected!");
    }
  } else {
    // flex values are not fluctuating rapidly, check if seizure is ongoing
    if (flexseizureDetected && millis() - seizureStartTime > FLUCTUATION_TIME * 1000) {
      flexseizureDetected = false;
//      Serial.println("Seizure ended.");
    }
  }
  return flexseizureDetected;
}
boolean mpudetectSeizure() {
  mpu6050.update();
  prevAccX = accX;
  prevAccY = accY;
  prevAccZ = accZ;
  accX = mpu6050.getAccX();
  accY = mpu6050.getAccY();
  accZ = mpu6050.getAccZ();
  prevAccMag = accMag;
  accMag = sqrt(pow(accX,2) + pow(accY,2) + pow(accZ,2));
  if (accMag > maxAccMag) maxAccMag = accMag;
  if (accMag < minAccMag) minAccMag = accMag;
  currTime = millis();
  if (currTime - prevTime >= 100) {
    deltaAccMag = abs(accMag - prevAccMag);
//    Serial.println(deltaAccMag);
    if (deltaAccMag > threshold) {
      onsetDetected = true;
      
      onsetEventCount++;
      Serial.print("onsetEventCount: ");
      Serial.println(onsetEventCount);
    }
    prevTime = currTime;
  }
  if (onsetDetected) {
    onsetDetected = false;
    onsetTime = currTime;
    detectionTimer = 0;
  }
  if (currTime - onsetTime <= detectionDuration) {
    detectionTimer++;
    if (onsetEventCount >= onsetThresholdCount) {
      seizureDetected = true;
      seizureCount++;
      Serial.print("Seizure detected! ");
      Serial.print("Count: ");
      Serial.println(seizureCount);
      onsetEventCount = 0;
      detectionTimer = 0;
      return true;
    }
  }
  else {
    onsetEventCount = 0;
    detectionTimer = 0;
  }
  return false;
}

void sendDataToThingSpeak(float voltage, float current, float energy, int motion, float cost) {

  if ((millis() - lastTime) > timerDelay) {
   
  // Set the fields and values to send to ThingSpeak
  ThingSpeak.setField(1, voltage);
  ThingSpeak.setField(2, current);
  ThingSpeak.setField(3, energy);
  ThingSpeak.setField(4, motion);
  ThingSpeak.setField(5, cost);

  // Send the data to ThingSpeak
  int response = ThingSpeak.writeFields(channel_id, api_key);

  // Check if the data was successfully sent
  if (response == 200) {
    Serial.println("Data sent to ThingSpeak");
  } else {
//    Serial.print("Error sending data to ThingSpeak. Response code: ");
//    Serial.println(response);
  }
  }
}

void loop() {
//    sendDataToThingSpeak(deltaAccMag, flexseizureDetectedFlag, mpuseizureDetectedFlag,deltaAccMag, onsetDetected);
  delay(200);
  // call detectSeizure() function and store seizure detected flag
    if (flexdetectSeizure()) {
    flexseizureDetectedFlag = true;
//    seizureTimer = millis(); // start the timer
    Serial.println("Seizure detected by flex sensor!");
  }

  if (mpudetectSeizure()) {
    mpuseizureDetectedFlag = true;
    Serial.println("Seizure detected by MPU6050!");
//    sendMessage("Seizure detected by MPU6050!");
  }
  delay(SAMPLE_RATE);
}
