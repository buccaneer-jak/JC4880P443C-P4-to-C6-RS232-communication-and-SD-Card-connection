# JC4880P443C-P4-to-C6-RS232-communication-and-SD-Card-connection
Test that the ESP32-P4 is communicating with the C6 and also checks the SD card connection.

Place the files into two different folders. Suggestions are ESP32-P4_WiFi_Controller and ESP32-C6_WiFi_OTA_Update_Handler

Copy the files ESP32-P4_WiFi_Controller.ino, SDMMC_ESP32P4.cpp and SDMMC_ESP32P4.h into the ESP32-P4_WiFi_Controller folder.

Copy the file ESP32-C6_WiFi_OTA_Updater_Handler.ino into the ESP32-C6_WiFi_OTA_Update_Handler folder.

For testing the RS232 communication between the P4 and C6 connect JP1

  pin 12, GPIO30 to pin 22, C6_U0TXD
  
  pin 14, GPIO29 to pin 20, C6_U0RXD

NOTE: I don't yet know if GPIO29 and GPIO30 are also used for other stuff but they work with this basic test.

Programming the ESP32-P4 is done using the Full-speed Type-C USB and then switching to the High-speed Type-C for the serial debugging.

Programming the ESP32-C6 is first done by connecting to JP1 pins 20 (C6_U0RXD) and pin 22 (C6_U0TXD) and placing the ESP32-C6 in Boot mode.

IMPORTANT: C6_U0TXD connects to the ESP-TXD and C6_U0RXD to the ESP-RXD

The ESP32-C6 can then be updated via OTA.

Espressif ESP-Prog board connections

![Guition-jc4880p433-Espressif ESP-Prog board](https://github.com/user-attachments/assets/fe238ee5-a183-4c14-a902-4a00e08d27df)


