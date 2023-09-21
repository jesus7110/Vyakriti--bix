#include <ESP8266WiFi.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <RF24.h>

#define SSID "POCO"
#define PASSWORD "pass"

#define GPS_TX_PIN 2   // Connect NEO-6M TX pin to ESP8266 RX pin
#define GPS_RX_PIN 3   // Connect NEO-6M RX pin to ESP8266 TX pin
#define NRF_CE_PIN 4   // NRF24L01 CE pin
#define NRF_CS_PIN 5   // NRF24L01 CS pin

RF24 radio(NRF_CE_PIN, NRF_CS_PIN);

TinyGPSPlus gps;
WiFiClient client;

const char* serverAddress = "your_server_ip_or_host";
const int serverPort = 80;

void setup() {
  Serial.begin(9600);  // Initialize serial communication
  Serial.println("Connecting to WiFi...");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  Serial.println("Connected to WiFi");
  
  radio.begin();
  radio.openWritingPipe(0xF0F0F0E1); // Set the address for communication
  radio.setPALevel(RF24_PA_HIGH);
  
  Serial.println("GPS-NRF24L01 Initialized");
}

void loop() {
  while (Serial.available() > 0) {
    gps.encode(Serial.read());
  }

  if (gps.location.isUpdated()) {
    float latitude = gps.location.lat();
    float longitude = gps.location.lng();
    String location = "Your Location";  // Replace with your location

    // Combine the GPS data and location string
    String dataToSend = String(latitude, 6) + "," + String(longitude, 6) + "," + location;
    
    // Ensure data length is 22 characters
    if (dataToSend.length() < 22) {
      dataToSend += "               ";  // Pad with spaces
      dataToSend = dataToSend.substring(0, 22); // Truncate or pad to 22 characters
    }

    // Send data via NRF24L01
    radio.stopListening();
    radio.write(dataToSend.c_str(), dataToSend.length() + 1);
    radio.startListening();

    // Send data to a server (optional)
    sendToServer(dataToSend);
  }
}

void sendToServer(String data) {
  if (client.connect(serverAddress, serverPort)) {
    client.print("POST /your-endpoint HTTP/1.1\r\n");
    client.print("Host: ");
    client.print(serverAddress);
    client.print("\r\n");
    client.print("Content-Type: application/x-www-form-urlencoded\r\n");
    client.print("Content-Length: ");
    client.print(data.length());
    client.print("\r\n\r\n");
    client.print(data);

    // Wait for server response or timeout
    long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println("Timeout");
        client.stop();
        return;
      }
    }

    // Read and print server response
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
    }
    client.stop();
  } else {
    Serial.println("Failed to connect to server");
  }
}
