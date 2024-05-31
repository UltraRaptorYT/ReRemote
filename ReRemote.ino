#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Bounce2.h>

#define IR_RECEIVE_PIN 5 // Using GPIO5 (D1) for the IR receiver pin
#define PLUS_BTN_PIN D7
#define MINUS_BTN_PIN D8

IRrecv irrecv(IR_RECEIVE_PIN);
decode_results results;

Bounce2::Button plusBtn = Bounce2::Button();
Bounce2::Button minusBtn = Bounce2::Button();

const uint32_t gate = 42069; // Gate ID
const unsigned long irValue = 0x4655434B; // IR Value

const char* ssid = "<WIFI_SSID>";          // Replace with your network SSID (name)
const char* password = "<WIFI_PASSWORD>";  // Replace with your network password

const char* host = "54.179.12.87";    // Replace with your server's hostname or IP
const uint16_t port = 42069;                // Replace with your server's port

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);


void sendTCPupdateState(WiFiClient &client, uint8_t networkVersion, uint8_t command, uint8_t state, uint32_t timestamp) {
  uint8_t versionCommand = (networkVersion << 4) | (command & 0x0F);

  byte gateBytes[2];
  gateBytes[0] = (gate >> 8) & 0xFF;
  gateBytes[1] = gate & 0xFF;

  byte timestampBytes[4];
  timestampBytes[0] = (timestamp >> 24) & 0xFF;
  timestampBytes[1] = (timestamp >> 16) & 0xFF;
  timestampBytes[2] = (timestamp >> 8) & 0xFF;
  timestampBytes[3] = timestamp & 0xFF;

  client.write(versionCommand);
  client.write(gateBytes, sizeof(gateBytes));
  client.write(state);
  client.write(timestampBytes, sizeof(timestampBytes));
}

// COMMAND 3 INCREMENT 4 DECREMENT
void updateCount(WiFiClient &client, uint8_t networkVersion, uint8_t command) {
  uint8_t versionCommand = (networkVersion << 4) | (command & 0x0F);

  byte gateBytes[2];
  gateBytes[0] = (gate >> 8) & 0xFF;
  gateBytes[1] = gate & 0xFF;

  client.write(versionCommand);
  client.write(gateBytes, sizeof(gateBytes));
  if (command == 3) {
    Serial.print("Increment");
  } else {
    Serial.print("Decrement");
  }
  Serial.print("Counter for ");
  Serial.println(gate);
}

void heartbeat(WiFiClient &client) {
  uint8_t networkVersion = 1;
  uint8_t command = 1; // Heartbeat 1

  uint8_t versionCommand = (networkVersion << 4) | (command & 0x0F);

  byte gateBytes[2];
  gateBytes[0] = (gate >> 8) & 0xFF;
  gateBytes[1] = gate & 0xFF;

  client.write(versionCommand);
  client.write(gateBytes, sizeof(gateBytes));
  Serial.print("Send heartbeat for ");
  Serial.println(gate);
}

void setup() {
  // For Btn
  plusBtn.attach(PLUS_BTN_PIN, INPUT);
  minusBtn.attach(MINUS_BTN_PIN, INPUT);
  plusBtn.interval(5);
  minusBtn.interval(5);
  plusBtn.setPressedState(LOW);
  minusBtn.setPressedState(LOW);

  Serial.begin(115200);
  delay(10);

  // Connect to Wi-Fi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  irrecv.enableIRIn();  // Start the IR receiver
  Serial.println("Begin Receiving IR");
}

void loop() {
  WiFiClient client;

  // Connect to the host
  Serial.print("Connecting to ");
  Serial.println(host);

  while (!client.connect(host, port)) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to ");
  Serial.println(host);

  uint32_t timestamp  = timeClient.getEpochTime();
  timeClient.update();

  if (irrecv.decode(&results)) {
    // Print the raw value in hexadecimal format
    Serial.print("Decoded raw data: ");
    Serial.println((unsigned long)(results.value), HEX);
    unsigned long receivedValue = (unsigned long)(results.value);

    // Command 2 = UpdateStatus
    // STATE 2 UNBLOCKED 3 BLOCKED
    // TIMESTAMP to represent time of request
    if (receivedValue == irValue) {
      sendTCPupdateState(client, 1, 2, 2, timestamp);
    }
    irrecv.resume();
  } else {
    Serial.println("No data from sensor");
//    sendTCPupdateState(client, 1, 2, 3, timestamp);
  }


  // Update Btn
  plusBtn.update();
  minusBtn.update();

  if (plusBtn.pressed()) {
    Serial.println("Plus");
    updateCount(client, 1, 3);
  }

  if (minusBtn.pressed()) {
    Serial.println("Minus");
    updateCount(client, 1, 4);
  }

  heartbeat(client);

  client.stop();
  Serial.print(host);
  Serial.println(" Connection closed");

  delay(100);
}
