#include "SDMMC_ESP32P4.h"

void SD_MMC_mount() {
	// Mount SD Card in 4-bit mode (default pins for ESP32-P4)
  if (!SD_MMC.begin("/sdcard", false)) {  // false = 4-bit mode, true = 1-bit mode
    Serial.println("❌ SD Card Mount FAILED!");
    Serial.println("Please check:");
    Serial.println("  - SD card is inserted");
    Serial.println("  - SD card is formatted (FAT32)");
    Serial.println("  - Card connections are correct");
  }
  else { 
    Serial.println("✅ SD Card Mounted, SUCCESS"); 
    SD_MMC_list_root_files(); 
    SD_MMC_Card_Type(); 
  }
}

void SD_MMC_Card_Type() {
  // Get SD Card info
  Serial.print("SD Card Type: ");
  uint8_t cardType = SD_MMC.cardType();
  switch (cardType) {
    case CARD_NONE:
      Serial.println("❌ No SD card attached");
      return;
    case CARD_MMC:
      Serial.println("MMC");
      break;
    case CARD_SD:
      Serial.println("SDSC");
      break;
    case CARD_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("UNKNOWN");
      break;
  }

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %llu MB\n", cardSize);
  Serial.printf("Total space: %llu MB\n", SD_MMC.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %llu MB\n",  SD_MMC.usedBytes() / (1024 * 1024));
  Serial.println();
}

void SD_MMC_list_root_files() {
	// List all files in root
  Serial.println("\n--- Root Directory Contents ---");
  listDir(SD_MMC, "/", 2);
}

// List directory contents
void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);
  
  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }
  
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}