#ifndef _SDMMC_ESP32P4_H
  #define _SDMMC_ESP32P4_H

  // SD Card Stuff
  #include "FS.h"
  #include "SD_MMC.h"

  void SD_MMC_mount();
  void SD_MMC_Card_Type();
  void SD_MMC_list_root_files();
  void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
  void createDir(fs::FS &fs, const char *path);
  void removeDir(fs::FS &fs, const char *path);
  void readFile(fs::FS &fs, const char *path);
  void writeFile(fs::FS &fs, const char *path, const char *message);
  void appendFile(fs::FS &fs, const char *path, const char *message);
  void renameFile(fs::FS &fs, const char *path1, const char *path2);
  void deleteFile(fs::FS &fs, const char *path);
  void testFileIO(fs::FS &fs, const char *path);

#endif
