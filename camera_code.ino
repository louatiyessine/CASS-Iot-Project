#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"           
#include "soc/rtc_cntl_reg.h"  

// ==========================================
// 1. WIFI SETTINGS
// ==========================================
const char* ssid = "Hints";
const char* password = "Hints+2025";

// ==========================================
// 2. PC SERVER SETTINGS 
// ==========================================
// ⚠️ Update this to your laptop's current IP!
const char* serverName = "10.10.120.11"; 
const int serverPort = 5000;
const char* serverPath = "/upload";

// ==========================================
// 3. CAMERA PINS
// ==========================================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Create a server that listens on Port 80
WiFiServer triggerServer(80);

void takeAndSendPhoto() {
  camera_fb_t * fb = NULL;
  
  // Capture
  fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Connect to PC Python Server
  WiFiClient client;
  Serial.print("Connecting to PC...");
  if (client.connect(serverName, serverPort)) {
    Serial.println("Connected! Sending...");
    
    client.print("POST ");
    client.print(serverPath);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(serverName);
    client.println("Content-Type: image/jpeg");
    client.print("Content-Length: ");
    client.println(fb->len);
    client.println(); 

    // Send Data in Chunks
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0; n<fbLen; n=n+1024) {
      if (n+1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      } else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        client.write(fbBuf, remainder);
      }
    }
    client.stop();
    Serial.println("Done.");
  } else {
    Serial.println("Failed to connect to PC.");
  }
  
  esp_camera_fb_return(fb);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  Serial.begin(115200);

  // CONFIG CAMERA
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()){
    config.frame_size = FRAMESIZE_SVGA; 
    config.jpeg_quality = 12;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // WIFI CONNECT
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Start Listening for Trigger
  triggerServer.begin();
  Serial.println("=============================================");
  Serial.print("WAITING FOR TRIGGER. IP: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/start");
  Serial.println("=============================================");
}

void loop() {
  WiFiClient triggerClient = triggerServer.available();

  if (triggerClient) {
    Serial.println("\n--- NEW CLIENT CONNECTED ---");
    Serial.println("Here is exactly what they said:");
    
    String currentLine = "";
    bool startSequence = false; 

    while (triggerClient.connected()) {
      if (triggerClient.available()) {
        char c = triggerClient.read();
        
        // 1. PRINT EVERYTHING TO SERIAL MONITOR (Debug Spy)
        Serial.write(c); 

        if (c == '\n') {
          if (currentLine.length() == 0) {
            // End of headers -> Send Response
            triggerClient.println("HTTP/1.1 200 OK");
            triggerClient.println("Content-type:text/html");
            triggerClient.println();
            triggerClient.print("<h1>Trigger Received!</h1>");
            break; 
          } else {
            // 2. CHECK FOR *ANY* GET REQUEST
            // This will trigger on "GET /start" OR just "GET /"
            if (currentLine.indexOf("GET") >= 0) {
              startSequence = true;
              Serial.println("\n>>> COMMAND DETECTED! (Trigger Set) <<<");
            }
            currentLine = ""; 
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    
    triggerClient.stop();
    Serial.println("\n--- CLIENT DISCONNECTED ---");
    
    // 3. START THE CAMERA SEQUENCE
    if (startSequence) {
       Serial.println("🚀 STARTING 5-PHOTO SEQUENCE NOW...");
       
       for(int i=1; i<=5; i++) {
          Serial.printf("📸 Taking Photo %d/5...\n", i);
          takeAndSendPhoto();
          
          if(i < 5) {
             Serial.println("⏳ Waiting 5s...");
             delay(5000);
          }
       }
       Serial.println("✅ Sequence Complete.");
    }
  }
}