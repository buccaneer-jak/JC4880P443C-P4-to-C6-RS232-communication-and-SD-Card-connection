#ifndef _SDMMC_ESP32P4_H
  #define _SDMMC_ESP32P4_H

  // SD Card Stuff
  #include "FS.h"
  #include "SD_MMC.h"

  void SD_MMC_mount();
  void SD_MMC_Card_Type();
  void SD_MMC_list_root_files();
  void listDir(fs::FS &fs, const char *dirname, uint8_t levels);

#endif