const int FLEX_PIN = 35;
const int THRESHOLD_LOW = 1450;
const int THRESHOLD_HIGH = 1550;
const int FLUCTUATION_TIME = 10; // time in seconds to check for fluctuations
const int FLUCTUATION_THRESHOLD = 60; // percentage of values that must be in fluctuation
const int MIN_READINGS = 15; // minimum number of readings required to trigger seizure detection
const int SAMPLE_RATE = 100; 
bool flexseizureDetected = false; // flag to indicate seizure detection
unsigned long seizureStartTime; // time seizure was first detected
int flexValues[FLUCTUATION_TIME * 1000 / SAMPLE_RATE]; // array to store flex sensor values

#include "WiFi.h"
WiFiClient client;
unsigned long lastTime = 0;
unsigned long timerDelay = 300;
const char* ssid = "sadguru Hostel _EXT";  // Replace with your network SSID
const char* password = "sadguru@1"; 

void setup() {
  Serial.begin(9600);
  while (!Serial) continue;
   // Connect or reconnect to WiFi
    if(WiFi.status() != WL_CONNECTED){
      Serial.print("Attempting to connect");
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, password); 
        delay(5000);     
      } 
      Serial.println("\nConnected.");
    }
}

bool detectSeizure() {
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
  Serial.print("Low count: ");
  Serial.print(lowCount);
  Serial.print(", high count: ");
  Serial.print(highCount);
  Serial.print(", low percent: ");
  Serial.print(lowPercent);
  Serial.print(", high percent: ");
  Serial.println(highPercent);
  
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

void loop() {
  // call detectSeizure() function and store seizure detected flag
  bool flexseizureDetectedFlag = detectSeizure();
  
  // do something with flexseizureDetectedFlag, e.g. send to remote server or trigger alarm
  // ...
//  Serial.println(flexseizureDetectedFlag);
  if (flexseizureDetectedFlag){Serial.println("Seizure detected!");}
  // delay to ensure a constant sampling rate
  delay(SAMPLE_RATE);
}
