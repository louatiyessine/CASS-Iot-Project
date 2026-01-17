#include <WiFi.h>
#include <HTTPClient.h>

// ==========================================
// 1. WIFI & CAMERA SETTINGS
// ==========================================
const char* ssid = "Hints";
const char* password = "Hints+2025";

// ✅ YOUR CAMERA IP
const char* cameraTriggerURL = "http://10.10.120.17/start"; 

// ==========================================
// 2. PIN DEFINITIONS
// ==========================================
const int PIN_IR = 18; 

// --- CONFIG ---
// Set to true if sensor sends 0 when obstacle detected (Standard)
const bool SENSOR_ACTIVE_LOW = true; 

void setup() {
  Serial.begin(115200);

  // 1. Setup Pin
  // Using INPUT_PULLUP prevents false triggers if wiring is loose
  pinMode(PIN_IR, INPUT_PULLUP);

  // 2. Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  Serial.println("\n✅ WiFi Connected!");
  Serial.println("System Ready. Waiting for movement...");
}

void loop() {
  // 1. Read Sensor
  int rawReading = digitalRead(PIN_IR);
  
  // Determine if object is present based on your sensor type
  bool objectDetected = (SENSOR_ACTIVE_LOW) ? (rawReading == LOW) : (rawReading == HIGH);

  if (objectDetected) {
    Serial.println("\n🚀 MOTION DETECTED! Triggering Camera...");

    // 2. Trigger the Camera
    if(WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(cameraTriggerURL);
      
      // Send the request
      int httpCode = http.GET();
      http.end(); // Close connection
      
      if(httpCode > 0) {
        Serial.println("✅ Signal Sent Successfully!");
      } else {
        Serial.print("❌ Error sending signal. Code: ");
        Serial.println(httpCode);
      }
    } else {
      Serial.println("⚠️ WiFi Disconnected! Cannot trigger.");
    }

    // 3. Wait to avoid spamming photos
    Serial.println("⏳ Waiting 5 seconds before next detection...");
    delay(5000); 
    Serial.println("READY.");
  }

  // Small delay to keep the loop stable
  delay(100);
}