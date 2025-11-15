// ESP32-C6 WiFi Handler Code with OTA Web Updater
// Receives commands from ESP32-P4 and handles WiFi operations

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Update.h>

// JC-ESP32P4-M3 SD Card Pin Definitions (SDMMC Mode)
// CLK:   GPIO43
// CMD:   GPIO44
// DATA0: GPIO39
// DATA1: GPIO40
// DATA2: GPIO42
// DATA3: GPIO41

// UART pins for communication with ESP32-P4
#define UART_RX_PIN 17 // C6 RX from P4 TX
#define UART_TX_PIN 16 // C6 TX to   P4 RX
#define UART_BAUD 115200

HardwareSerial P4Serial(1); // Use UART1

String commandBuffer = "";
WiFiClient wifiClient;

// Message ID tracking
uint32_t responseIdCounter = 0;

// OTA Web Server
WebServer otaServer(80);
bool otaServerStarted = false;

// OTA update credentials (change these for security!)
const char* otaUsername = "admin";
const char* otaPassword = "admin123";

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("ESP32-C6: Initializing WiFi handler with OTA...");
  
  // Initialize UART to ESP32-P4
  P4Serial.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  
  Serial.println("Serial TX pin: = " + String(TX));
  Serial.println("Serial RX pin: = " + String(RX));

  Serial.println("ESP32-C6: Ready to receive commands");
  sendAsyncNotification("C6_READY");
}

void loop() {
  // Handle OTA server if WiFi is connected
  if (WiFi.status() == WL_CONNECTED && otaServerStarted) {
    otaServer.handleClient();
  }
  
  // Read commands from ESP32-P4
  while (P4Serial.available()) {
    char c = P4Serial.read();
    if (c == '\n') {
      processCommand(commandBuffer);
      commandBuffer = "";
    } else {
      commandBuffer += c;
    }
  }
  
  // Monitor WiFi connection status
  static bool wasConnected = false;
  bool isConnected = WiFi.status() == WL_CONNECTED;
  
  if (wasConnected && !isConnected) {
    sendAsyncNotification("WIFI_DISCONNECTED");
    wasConnected = false;
    
    // Stop OTA server when WiFi disconnects
    if (otaServerStarted) {
      otaServer.stop();
      otaServerStarted = false;
      Serial.println("OTA server stopped");
    }
  } else if (!wasConnected && isConnected) {
    sendAsyncNotification("CONNECTED|IP:" + WiFi.localIP().toString());
    wasConnected = true;
    
    // Start OTA server when WiFi connects
    if (!otaServerStarted) {
      startOTAServer();
    }
  }
  
  delay(10);
}

// Initialize and start OTA Web Server
void startOTAServer() {
  // Root page - OTA upload form
  otaServer.on("/", HTTP_GET, []() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>ESP32-C6 OTA Update</title>";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "<style>";
    html += "body { font-family: Arial; margin: 20px; background: #f0f0f0; }";
    html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
    html += "h1 { color: #333; text-align: center; }";
    html += ".info { background: #e3f2fd; padding: 15px; border-radius: 5px; margin: 20px 0; }";
    html += ".info p { margin: 5px 0; }";
    html += "input[type=\"file\"] { width: 100%; padding: 10px; margin: 10px 0; }";
    html += "input[type=\"submit\"] { width: 100%; padding: 15px; background: #4CAF50; color: white; border: none; border-radius: 5px; font-size: 16px; cursor: pointer; }";
    html += "input[type=\"submit\"]:hover { background: #45a049; }";
    html += ".progress { width: 100%; height: 30px; background: #ddd; border-radius: 5px; margin: 20px 0; display: none; }";
    html += ".progress-bar { height: 100%; background: #4CAF50; border-radius: 5px; width: 0%; text-align: center; line-height: 30px; color: white; }";
    html += "</style></head><body>";
    html += "<div class=\"container\">";
    html += "<h1>ESP32-C6 OTA Updater</h1>";
    html += "<div class=\"info\">";
    html += "<p><strong>Device:</strong> ESP32-C6 WiFi Handler</p>";
    html += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
    html += "<p><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
    html += "<p><strong>Sketch Size:</strong> " + String(ESP.getSketchSize()) + " bytes</p>";
    html += "<p><strong>Free Space:</strong> " + String(ESP.getFreeSketchSpace()) + " bytes</p>";
    html += "</div>";
    html += "<form method=\"POST\" action=\"/update\" enctype=\"multipart/form-data\" id=\"upload_form\">";
    html += "<input type=\"file\" name=\"update\" accept=\".bin\" required>";
    html += "<input type=\"submit\" value=\"Update ESP32-C6 Firmware\">";
    html += "</form>";
    html += "<div class=\"progress\" id=\"progress\"><div class=\"progress-bar\" id=\"progress-bar\">0%</div></div>";
    html += "<script>";
    html += "document.getElementById(\"upload_form\").addEventListener(\"submit\", function(e) {";
    html += "  document.getElementById(\"progress\").style.display = \"block\";";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.upload.addEventListener(\"progress\", function(e) {";
    html += "    if (e.lengthComputable) {";
    html += "      var percent = (e.loaded / e.total) * 100;";
    html += "      document.getElementById(\"progress-bar\").style.width = percent + \"%\";";
    html += "      document.getElementById(\"progress-bar\").innerHTML = Math.round(percent) + \"%\";";
    html += "    }";
    html += "  });";
    html += "});";
    html += "</script>";
    html += "</div></body></html>";
    
    otaServer.send(200, "text/html", html);
  });
  
  // Handle firmware upload
  otaServer.on("/update", HTTP_POST, []() {
    otaServer.sendHeader("Connection", "close");
    if (Update.hasError()) {
      otaServer.send(500, "text/plain", "Update Failed!\n" + String(Update.errorString()));
    } else {
      otaServer.send(200, "text/plain", "Update Successful! Rebooting...");
      delay(1000);
      ESP.restart();
    }
  }, []() {
    HTTPUpload& upload = otaServer.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("OTA Update: %s\n", upload.filename.c_str());
      sendAsyncNotification("OTA_UPDATE_START|" + String(upload.filename));
      
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      } else {
        Serial.printf("OTA Progress: %d%%\n", (Update.progress() * 100) / Update.size());
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.printf("OTA Update Success: %u bytes\n", upload.totalSize);
        sendAsyncNotification("OTA_UPDATE_SUCCESS|" + String(upload.totalSize) + " bytes");
      } else {
        Update.printError(Serial);
        sendAsyncNotification("OTA_UPDATE_FAILED|" + String(Update.errorString()));
      }
    }
  });
  
  otaServer.begin();
  otaServerStarted = true;
  
  Serial.println("===========================================");
  Serial.println("OTA Web Server started!");
  Serial.println("Access at: http://" + WiFi.localIP().toString());
  Serial.println("Username: " + String(otaUsername));
  Serial.println("Password: " + String(otaPassword));
  Serial.println("===========================================");
  
  sendAsyncNotification("OTA_SERVER_STARTED|http://" + WiFi.localIP().toString());
}

// Generate unique response ID
uint32_t generateResponseId() {
  responseIdCounter++;
  if (responseIdCounter == 0) responseIdCounter = 1; // Avoid 0
  return responseIdCounter;
}

void processCommand(String fullCommand) {
  // Parse message ID from command
  // Format: [MsgID:0x########]COMMAND
  uint32_t reqMsgId = 0;
  String command = fullCommand;
  
  int msgIdStart = fullCommand.indexOf("[MsgID:0x");
  if (msgIdStart >= 0) {
    int msgIdEnd = fullCommand.indexOf("]", msgIdStart);
    if (msgIdEnd >= 0) {
      String msgIdStr = fullCommand.substring(msgIdStart + 9, msgIdEnd);
      reqMsgId = strtoul(msgIdStr.c_str(), NULL, 16);
      command = fullCommand.substring(msgIdEnd + 1);
      
      Serial.println("C6 received: Req[0x" + String(reqMsgId, HEX) + "] " + command);
    }
  } else {
    Serial.println("C6 received: " + command + " (no MsgID)");
  }
  
  if (command == "PING") {
    sendResponse("PONG", reqMsgId);
  }
  else if (command.startsWith("WIFI_CONNECT|")) {
    handleWiFiConnect(command, reqMsgId);
  }
  else if (command == "WIFI_DISCONNECT") {
    handleWiFiDisconnect(reqMsgId);
  }
  else if (command == "WIFI_STATUS") {
    handleWiFiStatus(reqMsgId);
  }
  else if (command == "OTA_STATUS") {
    handleOTAStatus(reqMsgId);
  }
  else if (command.startsWith("HTTP_GET|")) {
    handleHttpGet(command, reqMsgId);
  }
  else if (command.startsWith("HTTP_POST|")) {
    handleHttpPost(command, reqMsgId);
  }
  else {
    sendResponse("ERROR|Unknown command", reqMsgId);
  }
}

void handleWiFiConnect(String command, uint32_t reqMsgId) {
  // Parse: WIFI_CONNECT|SSID|PASSWORD
  int firstPipe = command.indexOf('|');
  int secondPipe = command.indexOf('|', firstPipe + 1);
  
  if (firstPipe == -1 || secondPipe == -1) {
    sendResponse("ERROR|Invalid WIFI_CONNECT format", reqMsgId);
    return;
  }
  
  String ssid = command.substring(firstPipe + 1, secondPipe);
  String password = command.substring(secondPipe + 1);
  
  Serial.println("Connecting to WiFi: " + ssid);
  
  WiFi.begin(ssid.c_str(), password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.println("IP: " + WiFi.localIP().toString());
    sendResponse("CONNECTED|IP:" + WiFi.localIP().toString(), reqMsgId);
  } else {
    Serial.println("\nConnection failed!");
    sendResponse("ERROR|Connection failed", reqMsgId);
  }
}

void handleWiFiDisconnect(uint32_t reqMsgId) {
  WiFi.disconnect();
  sendResponse("OK|Disconnected", reqMsgId);
}

void handleWiFiStatus(uint32_t reqMsgId) {
  if (WiFi.status() == WL_CONNECTED) {
    String status = "CONNECTED|IP:" + WiFi.localIP().toString() + 
                    "|RSSI:" + String(WiFi.RSSI()) + "dBm";
    sendResponse(status, reqMsgId);
  } else {
    sendResponse("DISCONNECTED", reqMsgId);
  }
}

void handleOTAStatus(uint32_t reqMsgId) {
  if (otaServerStarted && WiFi.status() == WL_CONNECTED) {
    String status = "OTA_RUNNING|URL:http://" + WiFi.localIP().toString();
    sendResponse(status, reqMsgId);
  } else {
    sendResponse("OTA_STOPPED", reqMsgId);
  }
}

void handleHttpGet(String command, uint32_t reqMsgId) {
  // Parse: HTTP_GET|URL
  int pipePos = command.indexOf('|');
  if (pipePos == -1) {
    sendResponse("ERROR|Invalid HTTP_GET format", reqMsgId);
    return;
  }
  
  String url = command.substring(pipePos + 1);
  
  if (WiFi.status() != WL_CONNECTED) {
    sendResponse("ERROR|WiFi not connected", reqMsgId);
    return;
  }
  
  HTTPClient http;
  http.begin(url);
  
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      sendResponse("OK|" + payload, reqMsgId);
    } else {
      sendResponse("ERROR|HTTP " + String(httpCode), reqMsgId);
    }
  } else {
    sendResponse("ERROR|HTTP request failed", reqMsgId);
  }
  
  http.end();
}

void handleHttpPost(String command, uint32_t reqMsgId) {
  // Parse: HTTP_POST|URL|DATA
  int firstPipe = command.indexOf('|');
  int secondPipe = command.indexOf('|', firstPipe + 1);
  
  if (firstPipe == -1 || secondPipe == -1) {
    sendResponse("ERROR|Invalid HTTP_POST format", reqMsgId);
    return;
  }
  
  String url = command.substring(firstPipe + 1, secondPipe);
  String data = command.substring(secondPipe + 1);
  
  if (WiFi.status() != WL_CONNECTED) {
    sendResponse("ERROR|WiFi not connected", reqMsgId);
    return;
  }
  
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.POST(data);
  
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
      String payload = http.getString();
      sendResponse("OK|" + payload, reqMsgId);
    } else {
      sendResponse("ERROR|HTTP " + String(httpCode), reqMsgId);
    }
  } else {
    sendResponse("ERROR|HTTP request failed", reqMsgId);
  }
  
  http.end();
}

// Send response with both response ID and request ID
void sendResponse(String response, uint32_t reqMsgId) {
  uint32_t respId = generateResponseId();
  
  // Format: [RespID:0x########][ReqID:0x########]RESPONSE_DATA
  String respIdPrefix = "[RespID:0x" + String(respId, HEX) + "]";
  String reqIdPrefix = "[ReqID:0x" + String(reqMsgId, HEX) + "]";
  String fullResponse = respIdPrefix + reqIdPrefix + response;
  
  P4Serial.print(fullResponse);
  P4Serial.print("\n");
  
  Serial.println("ESP32-C6 sending: Resp_MSGId for req[0x" + String(reqMsgId, HEX) + "] is [0x" + String(respId, HEX) + "]");
  Serial.println("         Data: " + response);
}

// Send asynchronous notification (no request ID)
void sendAsyncNotification(String notification) {
  uint32_t respId = generateResponseId();
  
  // Format: [RespID:0x########][ReqID:0x00000000]NOTIFICATION_DATA
  String respIdPrefix = "[RespID:0x" + String(respId, HEX) + "]";
  String reqIdPrefix = "[ReqID:0x00000000]"; // 0 indicates async notification
  String fullNotification = respIdPrefix + reqIdPrefix + notification;
  
  P4Serial.print(fullNotification);
  P4Serial.print("\n");
  
  Serial.println("ESP32-C6 sending (async): Resp_MSGId [0x" + String(respId, HEX) + "] " + notification);
}
