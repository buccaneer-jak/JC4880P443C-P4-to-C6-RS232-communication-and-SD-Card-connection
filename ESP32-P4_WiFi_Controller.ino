// ESP32-P4 Main MCU Code
// Communicates with ESP32-C6 via UART for WiFi connectivity
// For GUITION JC4880P433

#include <Arduino.h>
#include "SDMMC_ESP32P4.h"

// SD Card Stuff
//#include "FS.h"
//#include "SD_MMC.h"

// JC-ESP32P4-M3 SD Card Pin Definitions (SDMMC Mode)
// CLK:   GPIO43
// CMD:   GPIO44
// DATA0: GPIO39
// DATA1: GPIO40
// DATA2: GPIO42
// DATA3: GPIO41

// UART pins for ESP32-P4 to ESP32-C6 communication
#define UART_TX_PIN 29 // P4 TX to C6 RX
#define UART_RX_PIN 30 // P4 RX to C6 TX
#define UART_BAUD 115200

HardwareSerial C6Serial(0); // Use UART1

// Command protocol
#define CMD_WIFI_CONNECT "WIFI_CONNECT"
#define CMD_WIFI_DISCONNECT "WIFI_DISCONNECT"
#define CMD_WIFI_STATUS "WIFI_STATUS"
#define CMD_HTTP_GET "HTTP_GET"
#define CMD_HTTP_POST "HTTP_POST"
#define RESP_OK "OK"
#define RESP_ERROR "ERROR"
#define RESP_CONNECTED "CONNECTED"
#define RESP_DISCONNECTED "DISCONNECTED"

String responseBuffer = "";
bool waitingForResponse = false;
unsigned long responseTimeout = 0;
const unsigned long TIMEOUT_MS = 10000;

// Message ID tracking
uint32_t messageIdCounter = 0;
uint32_t lastSentMsgId = 0;

void setup() {
  Serial.begin(115200);
  delay(4500);
  Serial.println("ESP32-P4: Initializing...");

  Serial.println("TX pin: = " + String(TX));
  Serial.println("RX pin: = " + String(RX));
  
  // Initialize UART to ESP32-C6
  C6Serial.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  delay(500);
  
  Serial.println("ESP32-P4: UART to C6 initialized");
  Serial.println("ESP32-P4: Ready to communicate with ESP32-C6");
  
  // Test P4<->C6 connection
  if (testConnection()) {
    Serial.println("✅ ESP32-P4: C6 connection verified!");
  } else {
    Serial.println("❌ ESP32-P4: WARNING - No response from C6");
  }

  // Mount SD Card in 4-bit mode (default pins for ESP32-P4)
  SD_MMC_mount();
}

void loop() {
  // Example: Connect to WiFi
  static bool wifiConnected = false;
  static unsigned long lastCheck = 0;
  
  if (!wifiConnected && millis() - lastCheck > 5000) {
    Serial.println("\n--- Attempting WiFi Connection ---");
    if (connectToWiFi("YourSSID", "YourPassword")) {
    
      Serial.println("WiFi connected successfully!");
      wifiConnected = true;
      
      // Example: Make HTTP GET request
      delay(2000);
      String response = httpGet("http://api.ipify.org?format=json");
      Serial.println("HTTP Response: " + response);
    } else {
      Serial.println("WiFi connection failed");
    }
    lastCheck = millis();
  }
  
  // Check for status updates from C6
  readC6Response();
  
  // Check WiFi status periodically
  if (wifiConnected && millis() - lastCheck > 30000) {
    String status = getWiFiStatus();
    Serial.println("WiFi Status: " + status);
    lastCheck = millis();
  }
  
  delay(100);
}

// Generate unique message ID
uint32_t generateMessageId() {
  messageIdCounter++;
  if (messageIdCounter == 0) messageIdCounter = 1; // Avoid 0
  return messageIdCounter;
}

// Test connection to ESP32-C6
bool testConnection() {
  sendCommand("PING");
  String response = waitForResponse(2000);
  return response.indexOf("PONG") >= 0;
}

// Connect to WiFi network
bool connectToWiFi(const char* ssid, const char* password) {
  String cmd = String(CMD_WIFI_CONNECT) + "|" + ssid + "|" + password;
  sendCommand(cmd);
  
  String response = waitForResponse(TIMEOUT_MS);
  return response.indexOf(RESP_CONNECTED) >= 0;
}

// Disconnect from WiFi
bool disconnectWiFi() {
  sendCommand(CMD_WIFI_DISCONNECT);
  String response = waitForResponse(3000);
  return response.indexOf(RESP_OK) >= 0;
}

// Get WiFi connection status
String getWiFiStatus() {
  sendCommand(CMD_WIFI_STATUS);
  return waitForResponse(2000);
}

// HTTP GET request
String httpGet(const char* url) {
  String cmd = String(CMD_HTTP_GET) + "|" + url;
  sendCommand(cmd);
  return waitForResponse(TIMEOUT_MS);
}

// HTTP POST request
String httpPost(const char* url, const char* data) {
  String cmd = String(CMD_HTTP_POST) + "|" + url + "|" + data;
  sendCommand(cmd);
  return waitForResponse(TIMEOUT_MS);
}

// Send command to ESP32-C6 with unique message ID
void sendCommand(String command) {
  lastSentMsgId = generateMessageId();
  
  // Format: [MsgID:0x########]COMMAND
  String msgIdPrefix = "[MsgID:0x" + String(lastSentMsgId, HEX) + "]";
  String fullCommand = msgIdPrefix + command;
  
  C6Serial.print(fullCommand);
  C6Serial.print("\n");
  
  Serial.println("P4 -> C6: Req[0x" + String(lastSentMsgId, HEX) + "] " + command);
  
  responseBuffer = "";
  waitingForResponse = true;
  responseTimeout = millis() + TIMEOUT_MS;
}

// Wait for and return response from C6
String waitForResponse(unsigned long timeout) {
  unsigned long startTime = millis();
  responseBuffer = "";
  
  while (millis() - startTime < timeout) {
    if (C6Serial.available()) {
      char c = C6Serial.read();
      if (c == '\n') {
        String response = responseBuffer;
        responseBuffer = "";
        
        // Parse response to extract message IDs
        // Format: [RespID:0x########][ReqID:0x########]RESPONSE_DATA
        int respIdStart = response.indexOf("[RespID:0x");
        int reqIdStart = response.indexOf("[ReqID:0x");
        
        if (respIdStart >= 0 && reqIdStart >= 0) {
          int respIdEnd = response.indexOf("]", respIdStart);
          int reqIdEnd = response.indexOf("]", reqIdStart);
          
          String respIdStr = response.substring(respIdStart + 10, respIdEnd);
          String reqIdStr = response.substring(reqIdStart + 9, reqIdEnd);
          String responseData = response.substring(reqIdEnd + 1);
          
          Serial.println("C6 -> P4: Resp_MSGId for req[0x" + reqIdStr + "] is [0x" + respIdStr + "]");
          Serial.println("          Data: " + responseData);
          
          // Verify this response matches our request
          if (reqIdStr.equalsIgnoreCase(String(lastSentMsgId, HEX))) {
            return responseData;
          } else {
            Serial.println("         WARNING: Response MsgID mismatch! Expected 0x" + String(lastSentMsgId, HEX));
          }
        } else {
          Serial.println("C6 -> P4: " + response + " (no MsgID)");
        }
        
        return response;
      } else {
        responseBuffer += c;
      }
    }
    delay(10);
  }
  
  Serial.println("C6 -> P4: TIMEOUT for req[0x" + String(lastSentMsgId, HEX) + "]");
  return "TIMEOUT";
}

// Non-blocking read of C6 responses
void readC6Response() {
  while (C6Serial.available()) {
    char c = C6Serial.read();
    if (c == '\n') {
      if (responseBuffer.length() > 0) {
        // Parse async response
        int respIdStart = responseBuffer.indexOf("[RespID:0x");
        int reqIdStart = responseBuffer.indexOf("[ReqID:0x");
        
        if (respIdStart >= 0 && reqIdStart >= 0) {
          int respIdEnd = responseBuffer.indexOf("]", respIdStart);
          int reqIdEnd = responseBuffer.indexOf("]", reqIdStart);
          
          String respIdStr = responseBuffer.substring(respIdStart + 10, respIdEnd);
          String reqIdStr = responseBuffer.substring(reqIdStart + 9, reqIdEnd);
          String responseData = responseBuffer.substring(reqIdEnd + 1);
          
          Serial.println("C6 -> P4 (async): Resp_MSGId for req[0x" + reqIdStr + "] is [0x" + respIdStr + "]");
          Serial.println("                  Data: " + responseData);
          
          processAsyncResponse(responseData);
        } else {
          Serial.println("C6 -> P4 (async): " + responseBuffer);
          processAsyncResponse(responseBuffer);
        }
        
        responseBuffer = "";
      }
    } else {
      responseBuffer += c;
    }
  }
}

// Process asynchronous responses from C6
void processAsyncResponse(String response) {
  if (response.indexOf("WIFI_DISCONNECTED") >= 0) {
    Serial.println("*** WiFi connection lost! ***");
  } else if (response.indexOf("IP:") >= 0) {
    Serial.println("*** IP Address assigned: " + response + " ***");
  }
}