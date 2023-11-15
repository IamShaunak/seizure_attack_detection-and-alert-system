
#include <MPU6050_tockn.h>
#include <Wire.h>
#include <WiFi.h>
#include "ThingSpeak.h"

const char* ssid = "sadguru Hostel _EXT";   // your network SSID (name) 
const char* password = "sadguru@1";   // your network password

WiFiClient  client;

unsigned long myChannelNumber = 1;
const char * myWriteAPIKey = "K75F2IJ7KTVT8YD1";

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 300;

MPU6050 mpu6050(Wire);

void setup() {
  Serial.begin(9600);
  Wire.begin();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);
  WiFi.mode(WIFI_STA);   
  
  ThingSpeak.begin(client);
}

void loop() {

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
  
  mpu6050.update();
  Serial.print("angleX : ");
  Serial.print(mpu6050.getAngleX());
  Serial.print("\tangleY : ");
  Serial.print(mpu6050.getAngleY());
  Serial.print("\tangleZ : ");
  Serial.println(mpu6050.getAngleZ());

  int x_angle = mpu6050.getAngleX();
  int x = ThingSpeak.writeField(myChannelNumber, 1, x_angle, myWriteAPIKey);
  
  if(x == 200){
      Serial.println("Channel update successful.");
    }
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
    lastTime = millis();
  }
}
