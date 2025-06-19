#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// OLED Display config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Sensor Pins
#define PIR_PIN 14
#define TRIG_PIN 27
#define ECHO_PIN 26
#define RELAY_PIN 25

// Wi-Fi config
const char* ssid = "SYAKEER 8385";
const char* password = "keer2002";

// Firebase config
#define API_KEY "AIzaSyB3fAXrWgKzznj1duGyJUjL3R27CClI44Y"
#define DATABASE_URL "https://room-entry-logger-default-rtdb.asia-southeast1.firebasedatabase.app/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variables
int entryCount = 0;
long duration;
int distance;
int pirState = LOW;
int lastPirState = LOW;

// Server setup
WiFiServer server(80);

void displayMessage(String message, int count); // prototype

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  server.begin();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = "";
  auth.user.password = "";
  config.signer.tokens.legacy_token = "0ddZaO4nF5EDwScvS7SEhbADsqwUwjdghpNbRqeD";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (Firebase.ready()) {
    Serial.println("Firebase is ready.");
  } else {
    Serial.println("Firebase not ready!");
  }
}

void loop() {
  // PIR Sensor with state-change detection
  pirState = digitalRead(PIR_PIN);
  if (pirState == HIGH && lastPirState == LOW) {
    entryCount++;
    displayMessage("Motion Detected!", entryCount);
    Serial.println("Entry detected!");
    delay(500);
  }
  lastPirState = pirState;

  // Ultrasonic Sensor: Check distance
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) {
    distance = 0;
  } else {
    distance = duration * 0.034 / 2;
  }

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Distance: ");
  display.print(distance);
  display.println(" cm");

  display.setCursor(0, 12);
  display.print("Entries: ");
  display.println(entryCount);
  display.display();

  if (distance > 0 && distance < 20) {
    digitalWrite(RELAY_PIN, HIGH);
  } else {
    digitalWrite(RELAY_PIN, LOW);
  }

  // Send to Firebase with result check
  if (Firebase.RTDB.setInt(&fbdo, "/RoomLogger/entryCount", entryCount)) {
    Serial.println("Entry Count sent to Firebase.");
  } else {
    Serial.print("Failed to send Entry Count: ");
    Serial.println(fbdo.errorReason());
  }

  if (Firebase.RTDB.setInt(&fbdo, "/RoomLogger/distance", distance)) {
    Serial.println("Distance sent to Firebase.");
  } else {
    Serial.print("Failed to send Distance: ");
    Serial.println(fbdo.errorReason());
  }

  // Web server handler
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("GET / ") >= 0) {
      String response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
      response += "<!DOCTYPE html><html><head><title>Room Logger</title></head><body style='font-family:Arial;background:#121212;color:#f1f1f1;text-align:center;'>";
      response += "<h2>Room Logger System</h2>";
      response += "<p><strong>Entry Count:</strong> " + String(entryCount) + "</p>";
      response += "<p><strong>Distance:</strong> " + String(distance) + " cm</p>";
      response += "</body></html>";
      client.print(response);
    }

    client.stop();
  }

  delay(500);
}

// OLED motion message display
void displayMessage(String message, int count) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(message);
  display.setCursor(0, 20);
  display.print("Entries: ");
  display.println(count);
  display.display();
}
