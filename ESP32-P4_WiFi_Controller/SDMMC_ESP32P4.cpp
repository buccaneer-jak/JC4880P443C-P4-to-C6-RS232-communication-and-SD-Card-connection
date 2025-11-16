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

  uint64_t cardSize   = SD_MMC.cardSize()   / (1024 * 1024);
  uint64_t totalBytes = SD_MMC.totalBytes() / (1024 * 1024);
  uint64_t usedBytes  = SD_MMC.usedBytes()  / (1024 * 1024);
  uint64_t freeBytes  = totalBytes - usedBytes;
  
  Serial.printf("Card Size: %llu MB\n", cardSize);
  Serial.printf("Total: %llu MB\n", totalBytes);
  Serial.printf("Used:  %llu MB\n", usedBytes);
  Serial.printf("Free:  %llu MB\n", freeBytes);
  Serial.printf("Usage: %.1f%%\n",  (float)usedBytes / totalBytes * 100);
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
    Serial.println("❌ Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("❌ Not a directory");
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
// #########################################
// Create directory
void createDir(fs::FS &fs, const char *path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("✅ Dir created");
  } else {
    Serial.println("❌ mkdir failed");
  }
}

// Remove directory
void removeDir(fs::FS &fs, const char *path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("✅ Dir removed");
  } else {
    Serial.println("❌ rmdir failed");
  }
}

// Read file
void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\n", path);
  
  File file = fs.open(path);
  if (!file) {
    Serial.println("❌ Failed to open file for reading");
    return;
  }
  
  Serial.println("--- File Content ---");
  while (file.available()) {
    Serial.write(file.read());
  }
  Serial.println("--- End of File ---");
  file.close();
}

// Write to file (overwrites existing content)
void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);
  
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("❌ Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("✅ File written");
  } else {
    Serial.println("❌ Write failed");
  }
  file.close();
}

// Append to file
void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);
  
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("❌ Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("✅ Message appended");
  } else {
    Serial.println("❌ Append failed");
  }
  file.close();
}

// Rename file
void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("✅ File renamed");
  } else {
    Serial.println("❌ Rename failed");
  }
}

// Delete file
void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("✅ File deleted");
  } else {
    Serial.println("❌ Delete failed");
  }
}

// Test file I/O speed
void testFileIO(fs::FS &fs, const char *path) {
  File file = fs.open(path, FILE_WRITE);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  
  if (file) {
    len = file.write(buf, 512);
    file.close();
    end = millis() - start;
    Serial.printf("%u bytes written in %u ms\n", len, end);
  }
  
  file = fs.open(path);
  start = millis();
  end = start;
  if (file) {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read in %u ms\n", flen, end);
    file.close();
  }
}
