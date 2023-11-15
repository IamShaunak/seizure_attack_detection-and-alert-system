#include <Wire.h>
#include <MPU6050_tockn.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include "MAX30105.h"
#include "WiFi.h"
#include "ThingSpeak.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCROLL_SPEED 2   // Scroll speed in pixels per frame

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int scrollOffset = 0;

String phoneNumber = "+919172496290";
String apiKey = "1694291";
float temperature;
MPU6050 mpu6050(Wire);
bool flexseizureDetectedFlag = false;
bool mpuseizureDetectedFlag = false;
unsigned long seizureTimer = 0;
const unsigned long seizureTimeThreshold = 5000; // 5 seconds
//MPU
float accX, accY, accZ;
float prevAccX, prevAccY, prevAccZ;
float accMag, prevAccMag;
float deltaAccMag;
float maxAccMag, minAccMag;
float threshold = 0.02; // adjust this value to set the sensitivity of the seizure detection
unsigned long prevTime, currTime, onsetTime;
int seizureCount = 0;
boolean seizureDetected = false;
boolean onsetDetected = false;
int onsetEventCount = 0;
int detectionDuration = 10000; // detection duration in ms
int onsetThresholdCount = 3; // number of onset events required to trigger a seizure detection
int detectionTimer = 0;
//Flex
const int FLEX_PIN = 35;
const int FLEX_PIN2 = 34;
const int THRESHOLD_LOW = 1450;
const int THRESHOLD_HIGH = 1550;
const int FLUCTUATION_TIME = 10; // time in seconds to check for fluctuations
const int FLUCTUATION_THRESHOLD = 60; // percentage of values that must be in fluctuation
const int MIN_READINGS = 5; // minimum number of readings required to trigger seizure detection
const int SAMPLE_RATE = 500; 
bool flexseizureDetected = false; // flag to indicate seizure detection
unsigned long seizureStartTime; // time seizure was first detected
int flexValues[FLUCTUATION_TIME * 1000 / SAMPLE_RATE]; // array to store flex sensor values
//heart rate
#include "spo2_algorithm.h"

MAX30105 particleSensor;

const uint16_t BUFFER_LENGTH = 100;
uint32_t irBuffer[BUFFER_LENGTH]; // Infrared LED sensor data
uint32_t redBuffer[BUFFER_LENGTH]; // Red LED sensor data
int32_t spo2; // SPO2 value
int8_t validSPO2; // Indicator for valid SPO2 calculation
int32_t heartRate; // Heart rate value
int8_t validHeartRate; // Indicator for valid heart rate calculation

//const char* ssid = "sadguru Hostel _EXT";  // Replace with your network SSID
//const char* password = "sadguru@1"; 
const char* ssid = "vedh";  // Replace with your network SSID
const char* password = "vedh1234"; 
//const char* ssid = "Redmi Note 10S";  // Replace with your network SSID
//const char* password = "kisikonahimilega";  // Replace with your network password
const char* api_key = "K75F2IJ7KTVT8YD1";  // Replace with your ThingSpeak API key
const unsigned long channel_id = 2111814;  // Replace with your ThingSpeak channel ID

WiFiClient client;
unsigned long lastTime = 0;
unsigned long timerDelay = 300;

void setup() {
  Serial.begin(9600);
  
//  WiFi.mode(WIFI_STA);
//  while (!Serial) continue;

  ThingSpeak.begin(client);
  
  Wire.begin(21, 22);
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);
  delay(2000);
  prevTime = millis();
  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
//   Set sensor settings
  particleSensor.setup();
  Serial.println();
  Serial.println(F("MAX30105 initialized. Place your finger on the sensor."));
  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

  // Clear the display buffer
  display.clearDisplay();

  delay(1000);
}
float lowPercent;
float highPercent;
bool flexdetectSeizure() {
  // read flex sensor value
  int flexValue = analogRead(FLEX_PIN);
  int flexValue2 = analogRead(FLEX_PIN2);
//  Serial.println(flexValue);
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
  lowPercent = (float)lowCount / sizeof(flexValues) * 100;
  highPercent = (float)highCount / sizeof(flexValues) * 100;
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
//      Serial.println("Seizure detected!");
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

void para_display()
{
//  // Clear the display buffer
//  display.clearDisplay();
//
//  // Display sensor values
//  display.setTextSize(2);
//  display.setTextColor(SSD1306_WHITE);
//  
//  // Calculate the horizontal position of the text based on the scroll offset
//  int xPos = -scrollOffset;
//  
//  // Display sensor 1 value
//  display.setCursor(xPos, 0);
//  display.print("HR: ");
//  display.print(heartRate);
//  
//  // Display sensor 2 value
//  display.setCursor(xPos, 16);
//  display.print("spo2: ");
//  display.print(spo2);
//  
//  // Display sensor 3 value
//  display.setCursor(xPos, 32);
//  display.print("mpu: ");
//  display.print(mpuseizureDetectedFlag);
//
//  // Update the display
//  display.display();
//
//  // Scroll the text by incrementing the scroll offset
//  scrollOffset += SCROLL_SPEED;
//  
//  // If the scroll offset exceeds the text width, reset it to start from the beginning
//  if (scrollOffset >= SCREEN_WIDTH) {
//    scrollOffset = 0;
//  }
//
//  // Delay to control the scroll speed
//  delay(50);
  // Clear the display buffer
  display.clearDisplay();

  // Display sensor values
//  display.setTextSize(2);
//  display.setTextColor(SSD1306_WHITE);
//  display.setCursor(0, 0);
//  display.print("HR: ");
//  display.print(heartRate);
//  display.print("spo2: ");
//  display.print(spo2);
//  display.print("mpu: ");
//  display.print(mpuseizureDetectedFlag);

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("HR: ");
  display.setCursor(64, 0);
  display.println(heartRate,1);
  display.setCursor(0, 24);
  display.print("spo2: ");
  display.setCursor(64, 24);
  display.println(spo2,1);
  display.setCursor(0, 48);
  display.print("temp : ");
  display.setCursor(64, 48);
  display.println(temperature,1);

  // Update the display
  display.display();

  // Delay before updating sensor values and refreshing the display
  delay(500);
}

void loop() {
    para_display();
//    sendDataToThingSpeak(deltaAccMag, heartRate, spo2,temperature, mpuseizureDetectedFlag);
    String s;
    s= "Heart rate "+String(heartRate)+" SPO2 "+String(spo2);
  // check if flex sensor detects a seizure
  if (flexdetectSeizure()) {
    flexseizureDetectedFlag = true;
    seizureTimer = millis(); // start the timer
    Serial.println("Seizure detected by flex sensor!");
  }
  else
  {
    flexseizureDetectedFlag = false;
  }

  // check if MPU6050 detects a seizure
  if (mpudetectSeizure()) {
    mpuseizureDetectedFlag = true;
    Serial.println("Seizure detected by MPU6050!");
    sendMessage("Seizure detected by MPU6050! "+ s);
  }
  else
  {
    mpuseizureDetectedFlag = false;
  }
  calculate();
  // check if both sensors detected a seizure within the time threshold
  if (flexseizureDetectedFlag && mpuseizureDetectedFlag && (millis() - seizureTimer < seizureTimeThreshold)) {
    Serial.println("Seizure detected!");
    // do something here, like sending an alert or triggering an action
    // reset flags and timer
    flexseizureDetectedFlag = false;
    mpuseizureDetectedFlag = false;
    seizureTimer = 0;
//    if(mpuseizureDetectedFlag){
//      sendMessage("Hello from ESP32!");
//    }
  
  Serial.println();
  delay(SAMPLE_RATE);
}
}
void calculate(){
  // Read sensor data
  for (uint16_t i = 0; i < BUFFER_LENGTH; i++) {
    while (!particleSensor.available()) {
      particleSensor.check();
    }

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();

  }
  temperature = particleSensor.readTemperature();
  Serial.print("temperatureC=");
  Serial.print(temperature, 4);

  // Calculate heart rate and SpO2
  maxim_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_LENGTH, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  Serial.print(F("Heart Rate: "));
  Serial.print(heartRate);
  Serial.print(F(" bpm, SpO2: "));
  Serial.print(spo2);
  Serial.println(F(" %"));

  delay(1000);
}

void sendDataToThingSpeak(float voltage, int current, int energy, bool motion, bool cost) {

  if ((millis() - lastTime) > timerDelay) {
    // Connect or reconnect to WiFi
    if(WiFi.status() != WL_CONNECTED){
      Serial.print("Attempting to connect");
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, password); 
        delay(5000);     
      } 
      Serial.println("\nConnected.");
    }
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

void sendMessage(String message){

  // Data to send with HTTP POST
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(message);    
  HTTPClient http;
  http.begin(url);

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Send HTTP POST request
  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200){
    Serial.print("Message sent successfully");
  }
  else{
    Serial.println("Error sending the message");
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  }

  // Free resources
  http.end();
}
