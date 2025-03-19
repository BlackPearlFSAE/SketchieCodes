// For ESP32 only, preferably ESP32S3devkitc1 refers to this custom PCB repository <!-- Add link here -->, 
// ESP32 arduino core has exelent builtin example for using filesystem and SD card , both for SPI and MMC interface. 
// This sketch will show case .csv string writing with C languague function. and basic SDcard_logging.

// csv file template refers to: https://mailkmuttacth-my.sharepoint.com/:x:/g/personal/natdanai_chin_kmutt_ac_th/ETZwCgSxlotOhV5ipbpc344BMiCg92ylTzTJ4wvhxt0Rzw?e=v4bMgf

// SD card used, Sandisk Ultra 128 GB
// File format .csv , 2 files : firstFloor_log.csv , secondFloor_log.csv

// Memory usage:
// On ESP32S3devkitc-1: 27% of program storage space and 5% of dynamic memory occupied

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <driver/twai.h>

// // SD Card pins for LilyGO board
// #define SD_SCK  14
// #define SD_MISO 2
// #define SD_MOSI 15
// #define SD_CS   13

// // SD Card pins for ESp32S3devkitc1
// #define SD_SCK  12
// #define SD_MISO 13
// #define SD_MOSI 11
// #define SD_CS   10

#define SD_SCK  39
#define SD_MISO 40  
#define SD_MOSI 41
#define SD_CS   38

// CAN/TWAI bus pins - adjust to match your CAN transceiver connections
#define TWAI_TX_PIN GPIO_NUM_36  // Connect to CAN transceiver TX
#define TWAI_RX_PIN GPIO_NUM_35  // Connect to CAN transceiver RX

// Battery system configuration
#define NUM_MODULES 6
#define CELLS_PER_MODULE 10

// Data logging configuration
#define LOG_INTERVAL_MS 500  // How often to write summary data (every 5 seconds)
#define CSV_BMU_package "/firstFloor_log.csv"
#define CSV_AMS_package "/secondFloor_log.csv"
#define CSV_BMU_HEADER "Time,bmu_id,bmu_volt,bmu_temp1,bmu_temp2,bmu_dv,bmu_connect,bmu_ready_chg,bmu_cell_in_balance,bmu_ov_crti,bmu_ov_warn,bmu_lv_crit,bmu_lv_warn,bmu_ovt_crit,bmu_ovt_warn,bmu_ovd_crit,bmu_ovd_warn,Cell1,Cell2,Cell3,Cell4,Cell5,Cell6,Cell7,Cell8,Cell9,Cell10\n"
#define CSV_AMS_HEADER "Time,accel_ped1,accel_ped2,break_ped1,break_ped2,current_A,bspd_in,imd_in,air+,emr_o,obc_in,temp_light,lv_light,ams_ok,ams_volt,ams_ov_crit,ams_ov_wanr,ams_lv_crit,ams_lv_warn,ams_ovt_crit,ams_ovt_warn,ams_ovd_crit,ams_ovd_warn\n"

// CAN message IDs - adjust these to match your BMU's IDs
#define BMU_STATUS_ID 0x123  // Example ID - replace with your actual ID

// Battery data structure
struct BatteryData {
  bool chargingMode;
  uint16_t balancingCells;
  float moduleVoltage;
  float cellVoltageDiff;
  float temp1;
  float temp2;
} batteryData;

// Variables
unsigned long logCount = 0;
unsigned long lastStatTime = 0;
unsigned long lastLogTime = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  // Initialize SD card
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  // Create log file with headers
  if (!SD.exists(CSV_BMU_package)) {
    Serial.println("Creating new log file with headers");
    writeFile(SD, CSV_BMU_package, CSV_BMU_HEADER);
  } else {
    Serial.println("Log file exists, appending data");
  }

  // Create log file with headers
  if (!SD.exists(CSV_AMS_package)) {
    Serial.println("Creating new log file with headers");
    writeFile(SD, CSV_AMS_package, CSV_AMS_HEADER);
  } else {
    Serial.println("Log file exists, appending data");
  }
  
}

void loop() {

  // Log data periodically
  unsigned long currentMillis = millis();
  if (currentMillis - lastLogTime >= LOG_INTERVAL_MS) {
    lastLogTime = currentMillis;
    logBatteryData();
    log2ndFloordata();
  }
}

void logBatteryData() {
  // Get timestamp
  unsigned long timestamp = millis();
  char dataString[100];
  sprintf(dataString, "%lu,%lu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,", 
          1000000UL, 
          0xFFFFUL, 
          210,
          60, 
          65, 
          2, 
          0,
          0,
          0x0F,
          0xFFFF,
          0xFFFF,
          0xFFFF,
          0xAAAA,
          0xEEEE,
          0xCCCC,
          0xBBBB
        );
    
    int arry[10] = {1,2,3,4,5,6,7,8,9,10};
    char dataString2[50];  // Large enough to hold all numbers and commas
    int offset = 0;  // Keeps track of the write position

    for(int j = 0; j < 10; j++) {
      //sprintf will return the total number of character written into bufferr string , so offset will reflects that
        offset += snprintf(dataString2 + offset, sizeof(dataString2) - offset, "%d,", arry[j]);
    }

    // Remove the last comma (optional) as \n line break
    if (offset > 0){
      dataString2[offset - 1] = '\n';
    } 
        
  // Log to SD card
  appendFile(SD, CSV_BMU_package, dataString);
  appendFile(SD, CSV_BMU_package, dataString2);
  
  logCount++;
}

void log2ndFloordata(){
  unsigned long timestamp = millis();

  char dataString[100];
  sprintf(dataString, "%lu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", 
          timestamp, 
          2551 ,
          2552 ,
          2553,
          2554,
          2555,
          2556,
          0,
          0,
          0,
          1,
          1,
          1,
          0,
          0,
          1,
          1,
          1,
          1,
          0,
          0,
          0,
          0
        );
  
  // Log to SD card
  appendFile(SD, CSV_AMS_package, dataString);
  logCount++;
}

// SD card functions
void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    // Success - silent operation for performance
    Serial.println("Apeend success");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}