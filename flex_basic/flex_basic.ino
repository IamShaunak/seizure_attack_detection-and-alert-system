//For anyone else having this problem, you must attach the sensor analog read to a ADC1 pin on the esp. Analog doesnt 
//work on other GPIO pins when WiFi is enabled.

#include <WiFi.h>

const char* ssid = "sadguru Hostel _EXT";  // Replace with your network SSID
const char* password = "sadguru@1"; 

const int flexPin = 35;  // Pin connected to the flex sensor

void setup() {
  Serial.begin(115200);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
}

void loop() {
  // Read flex sensor value
  int flexValue = analogRead(flexPin);
  
  // Print the flex sensor value
  Serial.print("Flex Sensor Value: ");
  Serial.println(flexValue);

  delay(1000);
}
