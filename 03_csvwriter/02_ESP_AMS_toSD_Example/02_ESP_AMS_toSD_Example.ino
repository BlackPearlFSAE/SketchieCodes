// For ESP32 only, preferably ESP32S3devkitc1 refers to this custom PCB repository <!-- Add link here -->, 
// ESP32 arduino core has exelent builtin example for using filesystem and SD card , both for SPI and MMC interface. 
// More importantly its CAN controller interface TWAI, is used for interfacing with BMS internal bus, therefore can log data directly

// SD card used, Sandisk Ultra 128 GB
// File format .csv , 2 files : firstFloor_log.csv , secondFloor_log.csv

// Memory usage:
// On ESP32S3devkitc-1: 46 of program storage space and 7% of dynamic memory occupied

/************************* Includes ***************************/
#include <driver/gpio.h>
#include <driver/twai.h>       
#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <iostream>
#include <stdio.h>
#include <cstring>

/************************* Define macros *****************************/
// CAN/TWAI bus pins - For LilyGo
// #define TWAI_TX_PIN GPIO_NUM_5  // Connect to CAN transceiver TX
// #define TWAI_RX_PIN GPIO_NUM_4  // Connect to CAN transceiver RX

// CAN/TWAI bus pins - For ESP32S3 (Custom Board)
#define TWAI_TX_PIN GPIO_NUM_13  // Connect to CAN transceiver TX
#define TWAI_RX_PIN GPIO_NUM_14  // Connect to CAN transceiver RX

// ============ AMS data
// Containing Different Object representing the whole Electrical System

  // Default configuration of AMS
  #define CELL_NUM 10
  #define BMU_NUM 6 
  /*Amita Battery*/
  #define VMAX_CELL 4.2
  #define VMIN_CELL 3.2
  /*Thermistor*/
  #define TEMP_MAX_CELL 60
  #define TEMP_SENSOR_NUM 2
  #define DVMAX 0.2

  // AMS Communication
  // #define STANDARD_BIT_RATE 250E3
  #define DISCONNENCTION_TIMEOUT 650
  #define BCU_ADD 0x7FF
  #define OBC_ADD 0x1806E5F4

  twai_message_t receivedMessage;
  twai_message_t J1938msg;

  // Software Timer
  unsigned long lastlogtime1 = 0; 
  unsigned long lastStatTime = 0; 
  unsigned long communication_timer1 = 0;
  unsigned long shutdown_timer1 = 0;
  unsigned long lastModuleResponse[BMU_NUM];

  struct BMUdata {
    // Basic BMU Data
    uint32_t bmu_id = 0x00;
    uint8_t V_CELL[CELL_NUM] = {0};
    uint8_t TEMP_SENSE[TEMP_SENSOR_NUM] = {0};
    uint8_t V_MODULE = 0;
    uint8_t DV = 0;
    uint16_t OVERVOLTAGE_WARNING = 0;
    uint16_t OVERVOLTAGE_CRITICAL = 0;  
    uint16_t LOWVOLTAGE_WARNING = 0;
    uint16_t LOWVOLTAGE_CRITICAL = 0; 
    uint16_t OVERTEMP_WARNING = 0;
    uint16_t OVERTEMP_CRITICAL = 0;
    uint16_t OVERDIV_VOLTAGE_WARNING = 0 ; // Trigger cell balancing of the cell at fault
    uint16_t OVERDIV_VOLTAGE_CRITICAL = 0; // Trigger Charger disable in addition to Cell balancing
    // Status
    uint16_t BalancingDischarge_Cells = 0;
    bool BMUconnected = 0;   // Default as Active true , means each BMU is on the bus
    bool BMUreadytoCharge = 0;
  }; 

  // ACCUMULATOR Data , Local to BCU (Make this a struct later , or not? , I don't want over access)
  struct AMSdata {

    float ACCUM_VOLTAGE = 0.0; 
    float ACCUM_MAXVOLTAGE = (VMAX_CELL * CELL_NUM * BMU_NUM); // Default value
    float ACCUM_MINVOLTAGE = (VMIN_CELL * CELL_NUM * BMU_NUM); // Defualt value assum 8 module
    // float ACCUM_MAXVOLTAGE = (0); // Default value
    // float ACCUM_MINVOLTAGE = (0); // Defualt value assum 8 module
    bool ACCUM_CHG_READY = 0;

    bool OVERVOLT_WARNING = 0;
    bool LOWVOLT_WARNING = 0;
    bool OVERTEMP_WARNING = 0;
    bool OVERDIV_WARNING = 0;

    bool OVERVOLT_CRITICAL = 0;
    bool LOWVOLT_CRITICAL =  0;
    bool OVERTEMP_CRITICAL = 0;
    bool OVERDIV_CRITICAL = 0;

    // bool AMS_OK = 0; // Use this for Active Low Output
    bool AMS_OK = 1; // Use this for Active High Output
  };

  // Physical condition of OBC On board charger
  struct OBCdata {
    uint16_t OBCVolt = 0;
    uint16_t OBCAmp = 0;
    uint8_t OBCstatusbit = 0 ;   // Saftety information
    bool OBC_OK = 1;
  };

  // Physical condition of SDC and LV Circuit
struct LVsignal {
  bool AIRplus = 1; // AIR+
  bool IMD_Relay = 1; // IMD_OUT
  bool BSPD_Relay = 1; // BSPD_OUT
  bool EMERGENCY_BUTTON = 1;
  bool OBC_AUX_INPUT = 0;
  bool Temperature_warning_led = 0;
  bool lowvoltage_warning_led = 0;

  // BSPDADCreadingStatus
  uint16_t BrakePressure1;
  uint16_t BrakePressure2;
  uint16_t AccelPedal1;
  uint16_t AccelPedal2;
  uint16_t CurrentSense;
};

  //==================================================== CAN bus Methods
  // For ID custom protocol
  struct extCANIDDecoded {
      uint8_t PRIORITY;
      uint8_t BASE_ID;
      uint8_t MSG_NUM;
      uint8_t SRC;
      uint8_t DEST;
  };
  //standard CAN edit by jackie
  struct StandardCANIDDecoded {
      uint8_t PRIORITY;
      uint8_t MSG_NUM;
      uint8_t SRC;
  };

// BMU data , Accumulator data structure, Sensing data.
BMUdata BMU_Package[BMU_NUM];
LVsignal Signal_Package;
AMSdata AMS_Package;
OBCdata OBC_Package;

// Alias names
bool &AMS_OK = AMS_Package.AMS_OK;
float &ACCUM_MAXVOLTAGE = AMS_Package.ACCUM_MAXVOLTAGE; 
float &ACCUM_MINVOLTAGE = AMS_Package.ACCUM_MINVOLTAGE;  

// Flags
volatile bool ISR_FLG1 = false;
volatile bool CAN_SEND_FLG2 = false;
bool CHARGER_PLUGGED = false;
bool CAN_TIMEOUT_FLG = false;
unsigned long logCount = 0;

// ===========================================================================================

// SD card pins for Custom ESP32S3
#define SD_SCK  39
#define SD_MISO 40
#define SD_MOSI 41
#define SD_CS   38

// SD Card pins for LilyGO board
// #define SD_SCK  14
// #define SD_MISO 2
// #define SD_MOSI 15
// #define SD_CS   13

// SD card pins for ESP32s3 devkitc1 official development board
// #define SD_SCK  12
// #define SD_MISO 13
// #define SD_MOSI 11
// #define SD_CS   10

// Data logging configuration
#define LOG_INTERVAL_MS 500  // How often to write summary data (every 5 seconds)
#define CSV_BMU_package "/firstFloor_log.csv"
#define CSV_AMS_package "/secondFloor_log.csv"
#define CSV_BMU_HEADER "Time,bmu_id,bmu_volt,bmu_temp1,bmu_temp2,bmu_dv,bmu_connect,bmu_ready_chg,bmu_cell_in_balance,bmu_ov_crti,bmu_ov_warn,bmu_lv_crit,bmu_lv_warn,bmu_ovt_crit,bmu_ovt_warn,bmu_ovd_crit,bmu_ovd_warn,Cell1,Cell2,Cell3,Cell4,Cell5,Cell6,Cell7,Cell8,Cell9,Cell10\n"
#define CSV_AMS_HEADER "Time,accel_ped1,accel_ped2,break_ped1,break_ped2,current_A,bspd_in,imd_in,air+,emr_o,obc_in,temp_light,lv_light,ams_ok,ams_volt,ams_ov_crit,ams_ov_wanr,ams_lv_crit,ams_lv_warn,ams_ovt_crit,ams_ovt_warn,ams_ovd_crit,ams_ovd_warn\n"

// #define mystring "LLL"
/**************** Local Function Delcaration *******************/
void processOBCmsg ( twai_message_t *receivedframe );
void processBMUmsg ( twai_message_t *receivedframe, BMUdata *BMS_ROSPackage);
void debugFrame();
bool checkModuleDisconnect(BMUdata *BMU_Package);
void twaiTroubleshoot();
void resetAllStruct();
void sensorReading();
void log1stFloordata(int moduleindex);
void log2ndFloordata();
unsigned char* splitHLbyte(unsigned int num);
unsigned int mergeHLbyte(unsigned char Hbyte, unsigned char Lbyte);
bool *toBitarrayMSB(uint16_t num);
bool *toBitarrayLSB(uint16_t num);
uint16_t toUint16FromBitarrayMSB(const bool *bitarr);
uint16_t toUint16FromBitarrayLSB(const bool *bitarr);
uint16_t createCANID(uint8_t PRIORITY, uint8_t SRC_ADDRESS, uint8_t MSG_NUM);
// Structure of CAN ID :: Receiver side
void decodeExtendedCANID(struct extCANIDDecoded* myCAN ,uint32_t canID);
void decodeStandardCANID(struct StandardCANIDDecoded *myCAN, uint32_t canID);


/*******************************************************************
  ==============================Setup==============================
********************************************************************/
void IRAM_ATTR onTimer1() {
  // May or may not be critical section , --- later to be thought out
  ISR_FLG1 = 1;
}

void setup() {
  Serial.begin(115200);
  /* CAN Communication Setup */
    receivedMessage.extd = false;
    J1938msg.extd = true;
    twai_general_config_t general_config = TWAI_GENERAL_CONFIG_DEFAULT(TWAI_TX_PIN, TWAI_RX_PIN, TWAI_MODE_NORMAL);
    general_config.tx_queue_len = 800; // worstcase is 152 bit/frame , this should hold about 5 frame
    general_config.rx_queue_len = 1300; // RX queue hold about 8 frame
    twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(); // May config this later
    // Install the TWAI driver
    if (twai_driver_install(&general_config, &timing_config, &filter_config) == !ESP_OK) {
      Serial.println("TWAI Driver install failed__");
      while(1);
    }
    // Start the TWAI driver
      if (twai_start() == ESP_OK) {
        Serial.println("TWAI Driver installed , started");
        // Reconfigure the alerts to detect the error of frames received, Bus-Off error and RX queue full error
        uint32_t alerts_to_enable = TWAI_ALERT_RX_DATA | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_QUEUE_FULL;
        if (twai_reconfigure_alerts(alerts_to_enable, NULL) == !ESP_OK) {
          Serial.println("Failed to reconfigure alerts");
          while(1);
        }
      }

  // Initialize SD card
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

/*******************************************************************
  =======================Mainloop===========================
********************************************************************/
  
void loop() {
  unsigned long currentMillis = millis();
  if (twai_receive(&receivedMessage, 1) == ESP_OK) {
    processBMUmsg(&receivedMessage, BMU_Package); // 200ms cycle & 500ms cycle of faultcode
    if(CHARGER_PLUGGED)
      processOBCmsg(&J1938msg); // Unpack CAN frame and insert to OBC_Package:  500ms cycle 
    // Update timeout flag and communication_timer
    CAN_TIMEOUT_FLG = false;
    communication_timer1 = millis();
  }
  // if No module is connected from CAN bus , run this code and return until the bus is active
  else if (currentMillis - communication_timer1 >= DISCONNENCTION_TIMEOUT){
    // Shutdown
    if( currentMillis - shutdown_timer1 >= 400 ){
        Serial.println("NO_BYTE_RECV");
        shutdown_timer1 = millis();
    }
    // resetAllStruct() only once
    if(CAN_TIMEOUT_FLG == false)
      resetAllStruct();
    CAN_TIMEOUT_FLG = true;
    return;
  }

  // In case if any module disconnect
  if(checkModuleDisconnect(BMU_Package) == 0){
    if( currentMillis - shutdown_timer1 >= 400 ){
      Serial.println("MODULE_DISCONNECT -- SHUTDOWN");
      for(int i = 0 ; i < BMU_NUM ; i++ ) {
        Serial.printf("BMU %d connect: ", i);
        Serial.printf("%d \n",BMU_Package[i].BMUconnected);
      }
        shutdown_timer1 = millis();
    }
    return;
  }
/*================================= Logger Function*/
  // Log data Every 200 ms
  if (currentMillis - lastlogtime1  >= LOG_INTERVAL_MS) {
    lastlogtime1 = currentMillis;
    // Log all BMU_Package
    for(int i =0; i < BMU_NUM ; i++) log1stFloordata(i);
    
    log2ndFloordata(); // Log AMS_Package
  }
} 

/* ==================================Main Local Functions==============================*/
bool isModuleActive(int moduleIndex); // Check the latest response from each module
void resetModuleData(int moduleIndex); // Reset that Module struct if !ismoduleActive
void packing_AMSstruct (int moduleIndex); // recalculate data to AMS struct in sync with number of Active BMU
void dynamicModulereset(BMUdata *BMU_Package);

void processBMUmsg ( twai_message_t* receivedframe , BMUdata *BMU_Package) {
  // Reset BMU struct Value
  dynamicModulereset(BMU_Package);

  // decodeCANID according to BP16 agreement
  StandardCANIDDecoded decodedCANID;
  decodeStandardCANID(&decodedCANID, (receivedframe->identifier) );
  
  // Distingush Module ID
    int i = decodedCANID.SRC - 1;
    if(i >= BMU_NUM) return;      
  
  // Mark timestamp of successfully received Module, No update for disconnected BMU.
  lastModuleResponse[i] = millis();

  /* ---------------- unpack ReceiveFrame to BMUframe ------------------- */
  /*  Message Priority 02 :: BMUModule & Cells data  */
  if(decodedCANID.PRIORITY == 0x02)
  {
    switch (decodedCANID.MSG_NUM) { 
      // MSG1 == Operation status
      case 1:
        // Charging Ready
        BMU_Package[i].BMUreadytoCharge = receivedframe->data[0];
        // Balancing Discharge cell number
        BMU_Package[i].BalancingDischarge_Cells = mergeHLbyte(receivedframe->data[1],receivedframe->data[2]);
        // Vbatt (Module) , dVmax(cell)
        BMU_Package[i].V_MODULE = receivedframe->data[3]; 
        BMU_Package[i].DV = receivedframe->data[4]; 
        // Temperature sensor
        BMU_Package[i].TEMP_SENSE[0] = receivedframe->data[5];
        BMU_Package[i].TEMP_SENSE[1] = receivedframe->data[6];
        break;

      case 2:
        // Low series Side Cell C1-C8
        for(short j=0; j< 8; j++)
          BMU_Package[i].V_CELL[j] = receivedframe->data[j];
        break;

      case 3:
        // High series side Cell C8-CellNumber
        for( short j = 8; j < CELL_NUM; j++ )
          BMU_Package[i].V_CELL[j] = receivedframe->data[(j-8)];
        break;
    }
  }
  /*  Message Priority 01 :: FaultCode  */
  else if(decodedCANID.PRIORITY == 0x01)
  {
    switch (decodedCANID.MSG_NUM) {
      case 1:
        // Merge H and L byte of each FaultCode back to 10 bit binary
        BMU_Package[i].OVERVOLTAGE_WARNING =  mergeHLbyte( receivedframe->data[0], receivedframe->data[1] );  
        BMU_Package[i].OVERVOLTAGE_CRITICAL = mergeHLbyte( receivedframe->data[2], receivedframe->data[3] );  
        BMU_Package[i].LOWVOLTAGE_WARNING = mergeHLbyte( receivedframe->data[4], receivedframe->data[5] );
        BMU_Package[i].LOWVOLTAGE_CRITICAL = mergeHLbyte( receivedframe->data[6], receivedframe->data[7] ); 
        break;
      case 2:
        // Merge H and L byte of each FaultCode back to 10 bit binary
        BMU_Package[i].OVERTEMP_WARNING = mergeHLbyte( receivedframe->data[0], receivedframe->data[1] );  
        BMU_Package[i].OVERTEMP_CRITICAL = mergeHLbyte( receivedframe->data[2], receivedframe->data[3] ); 
        BMU_Package[i].OVERDIV_VOLTAGE_WARNING = mergeHLbyte( receivedframe->data[4], receivedframe->data[5] ); 
        BMU_Package[i].OVERDIV_VOLTAGE_CRITICAL = mergeHLbyte( receivedframe->data[6], receivedframe->data[7] );
        break;
    }
  }
  
  // Reset Accumulator Parameter before dynamically recalculate based on BMU current state
  AMS_Package = AMSdata();
  // Pack BMUframe to AMSframe according to the following condition
  for(int j = 0; j <BMU_NUM ; j++)
  {
    // if Module is connected to CANbus, set as connect, and recalculateAMS package a new
    if(isModuleActive(j)){ 
        BMU_Package[j].BMUconnected = 1;
        packing_AMSstruct(j);
    } else {
        BMU_Package[j].BMUconnected = 0;
    }
  }
}

void processOBCmsg ( twai_message_t *J1939frame ) {
  // if message ID isnt 0x18FF50E5 , return
  if(J1939frame->identifier != 0x18FF50E5)
    return;
  
    // Monitor & Translate current Frame data
    uint8_t VoutH = J1939frame->data[0];
    uint8_t VoutL = J1939frame->data[1];
    uint8_t AoutH = J1939frame->data[2];
    uint8_t AoutL = J1939frame->data[3];
    OBC_Package.OBCstatusbit =  J1939frame->data[4]; // Status Byte
    OBC_Package.OBCVolt = mergeHLbyte(VoutH,VoutL);
    OBC_Package.OBCAmp = mergeHLbyte(AoutH,AoutL);

}
/* ==================================Sub Functions==============================*/
bool isModuleActive(int moduleIndex) {
  unsigned int MAX_SILENCE = DISCONNENCTION_TIMEOUT;
  return (millis() - lastModuleResponse[moduleIndex]) <= (MAX_SILENCE);
}
void resetModuleData(int moduleIndex){
  BMU_Package[moduleIndex].~BMUdata(); // Explicitly call destructor (optional)
  new (&BMU_Package[moduleIndex]) BMUdata();
}
void packing_AMSstruct (int moduleIndex) {
  int &i = moduleIndex;
  // Recalculate AMS based on current BMU states
  AMS_Package.ACCUM_VOLTAGE += ( static_cast<float> (BMU_Package[i].V_MODULE)) * 0.2;
  AMS_Package.OVERVOLT_WARNING |= BMU_Package[i].OVERVOLTAGE_WARNING;
  AMS_Package.LOWVOLT_WARNING |= BMU_Package[i].LOWVOLTAGE_WARNING;
  AMS_Package.OVERTEMP_WARNING |= BMU_Package[i].OVERTEMP_WARNING;
  AMS_Package.OVERDIV_CRITICAL |= BMU_Package[i].OVERDIV_VOLTAGE_WARNING;
  AMS_Package.OVERVOLT_CRITICAL |= BMU_Package[i].OVERVOLTAGE_CRITICAL;
  AMS_Package.LOWVOLT_CRITICAL |= BMU_Package[i].LOWVOLTAGE_CRITICAL;
  AMS_Package.OVERTEMP_CRITICAL |= BMU_Package[i].OVERTEMP_CRITICAL;

  AMS_Package.ACCUM_CHG_READY &= (BMU_Package[i].BMUreadytoCharge);
  AMS_Package.OVERDIV_CRITICAL |= BMU_Package[i].OVERDIV_VOLTAGE_CRITICAL;
  
  // Available Module , and 
}
void resetAllStruct(){
  for (int i = 0; i < BMU_NUM; i++){
    resetModuleData(i);
    lastModuleResponse[i] = 0;
  }
  AMS_Package = AMSdata();
  OBC_Package = OBCdata();
}
bool checkModuleDisconnect(BMUdata *BMU_Package){

  bool disconnectedFromCAN = 1; 
  for(short i =0; i< BMU_NUM ; i++){
    if(BMU_Package[i].BMUconnected == 0)
      disconnectedFromCAN = 0; 
  }
    return disconnectedFromCAN;
}
void dynamicModulereset(BMUdata *BMU_Package){ 
  for(short i =0; i< BMU_NUM ; i++){
    // if any of the board aren't in connection => throw error
    if(BMU_Package[i].BMUconnected == 0){
      resetModuleData(i); // Reset that module data (revert voltage , temp., flags , etc. Back to zero)
    }   
  }
}

/* ==================================Serial Debugger==============================*/
void debugFrame(){
  Serial.printf("%X\n", receivedMessage.identifier);
  for (int i = 0; i < receivedMessage.data_length_code; i++) 
      Serial.printf("%X",receivedMessage.data[i]);
    Serial.println();
}

void debugOBCmsg(){ 
    Serial.print("Voltage from OBC: "); Serial.print(OBC_Package.OBCVolt); Serial.println("V");
    Serial.print("Current from OBC: "); Serial.print(OBC_Package.OBCAmp); Serial.println("A");
    Serial.print("OBC status bit"); Serial.println(OBC_Package.OBCstatusbit);

    // Intepret Individual bit meaning
    bool *obcstatbitarray =  toBitarrayLSB(OBC_Package.OBCstatusbit); // Status Byte
    
    Serial.print("OBC status bit: ");
    switch (obcstatbitarray[0]) {
      case 1:
        Serial.println("ChargerHW = Faulty");
        break;
    }
    switch (obcstatbitarray[1]) {
      case 1:
        Serial.println("ChargerTemp = Overheat");
        break;
    }
    switch (obcstatbitarray[2]) {
      case 1:
        Serial.println("ChargerACplug = Reversed");
        break;
    }
    switch (obcstatbitarray[3]) {
      case 1:
        Serial.println("Charger detects: NO BATTERY VOLTAGES");
        break;
    }
    switch (obcstatbitarray[4]) {
      case 1:
        Serial.println("OBC Detect COMMUNICATION Time out: (6s)");
        break;
    } 
}

void log1stFloordata(int moduleindex) {
  int &i = moduleindex;
  unsigned long timestamp = millis();

  char dataString1[100];
  // right now 17
  sprintf(dataString1, "%lu,%lu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u," , 
          timestamp, 
          BMU_Package[i].bmu_id, 
          BMU_Package[i].V_MODULE,
          BMU_Package[i].TEMP_SENSE[0], 
          BMU_Package[i].TEMP_SENSE[1], 
          BMU_Package[i].DV, 
          BMU_Package[i].BMUconnected,
          BMU_Package[i].BMUreadytoCharge,
          BMU_Package[i].BalancingDischarge_Cells,
          BMU_Package[i].OVERVOLTAGE_CRITICAL,
          BMU_Package[i].OVERVOLTAGE_WARNING,
          BMU_Package[i].LOWVOLTAGE_CRITICAL,
          BMU_Package[i].LOWVOLTAGE_WARNING,
          BMU_Package[i].OVERTEMP_CRITICAL,
          BMU_Package[i].OVERTEMP_WARNING,
          BMU_Package[i].OVERDIV_VOLTAGE_CRITICAL,
          BMU_Package[i].OVERDIV_VOLTAGE_WARNING
        );
      // THe actual size is 50 , but * 2 for fail safe

  // Second set of dataString hold cells data
  char dataString2[CELL_NUM];  
  int offset = 0;  // Track write position

  for(int j = 0; j < 10; j++) {
      char voltage = BMU_Package[i].V_CELL[j];
      offset += snprintf(dataString2 + offset, sizeof(dataString2) - offset, "%u,", voltage);
  }

  // Replace last comma as \n line break
  if (offset > 0){
    dataString2[offset - 1] = '\n';
  } 
        
  // Log both data string to SD card
  appendFile(SD, CSV_BMU_package, dataString1);
  appendFile(SD, CSV_BMU_package, dataString2);
  logCount++;
}

void log2ndFloordata(){
  unsigned long timestamp = millis();

  char dataString[100];
  sprintf(dataString, "%lu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", 
          timestamp, 
          Signal_Package.AccelPedal1 ,
          Signal_Package.AccelPedal2 ,
          Signal_Package.BrakePressure1,
          Signal_Package.BrakePressure2,
          Signal_Package.CurrentSense,
          Signal_Package.BSPD_Relay,
          Signal_Package.IMD_Relay,
          Signal_Package.AIRplus,
          Signal_Package.EMERGENCY_BUTTON,
          Signal_Package.OBC_AUX_INPUT,
          Signal_Package.Temperature_warning_led,
          Signal_Package.lowvoltage_warning_led,
          AMS_Package.AMS_OK,
          AMS_Package.ACCUM_VOLTAGE,
          AMS_Package.OVERVOLT_CRITICAL,
          AMS_Package.OVERVOLT_WARNING,
          AMS_Package.LOWVOLT_CRITICAL,
          AMS_Package.LOWVOLT_WARNING,
          AMS_Package.OVERTEMP_CRITICAL,
          AMS_Package.OVERTEMP_WARNING,
          AMS_Package.OVERDIV_CRITICAL,
          AMS_Package.OVERDIV_WARNING
        );
    
  // Log to SD card
  appendFile(SD, CSV_AMS_package, dataString);
  logCount++;
}
/* ==================================================== General Functions 
*/
// Split uint16_t to High byte and Low byte
unsigned char* splitHLbyte(unsigned int num){
  static uint8_t temp[2]; // initialize
  temp[0] = (num >> 8) & 255;  // Extract the high byte
  temp[1] = num & 255;         // Extract the low byte
  return temp;
}

// Merge 2 bytes into uint16_t
unsigned int mergeHLbyte(unsigned char Hbyte, unsigned char Lbyte){
  uint16_t temp = (Hbyte << 8) | Lbyte; // bitshiftLeft by 8 OR with low byte
  return temp;
}


// Convert N bit binary to 16 bit array from MSB-first (big endian) 
bool *toBitarrayMSB(uint16_t num){
  static bool bitarr[16]; // array to hold 8 binary number
  for (int i = 15; i >= 0; i--){
    bool bit = num & 1;
    bitarr[i] = bit;
    num >>= 1; // Right Shift num by 1 pos. before next loop , we AND with 1 again
  } 
  return bitarr; 
}

// Convert N bit binary to 16 bit array from LSB-first (little endian)
bool *toBitarrayLSB(uint16_t num){
  static bool bitarr[16]; // array to hold 8 binary number
  for (int i = 0; i < 16; i++){
    bool bit = num & 1;
    bitarr[i] = bit;
    num >>= 1; // Right Shift num by 1 pos. before next loop , we AND with 1 again
  } 
  return bitarr; 
}

// Convert MSB-first bit array back to uint16_t
uint16_t toUint16FromBitarrayMSB(const bool *bitarr) {
    uint16_t num = 0;
    for (int i = 0; i < 16; i++) {
        if (bitarr[i]) {
            num |= (1 << (15 - i));
        }
    }
    return num;
}

// Convert LSB-first bit array back to uint16_t
uint16_t toUint16FromBitarrayLSB(const bool *bitarr) {
    uint16_t num = 0;
    for (int i = 0; i < 16; i++) {
        if (bitarr[i]) {
            num |= (1 << i);
        }
    }
    return num;
}

/*=============================================================== CAN bus
-----( Check the spreadsheet in README.md for Communication Agreement )*/
// 1 CE 1 0A 00
// 0001 1100 1110 0001 0000 1010 0000 0000

uint16_t createCANID(uint8_t PRIORITY, uint8_t SRC_ADDRESS, uint8_t MSG_NUM) {
    uint16_t canID = 0;
    canID |= ((PRIORITY & 0x0F) << 8);        // (PP) Priority (bits 30–31)
    canID |= ((SRC_ADDRESS & 0x0F) << 4);      // (SS) Source address (bits 8–15)
    canID |= (MSG_NUM & 0x0F);            // (DD) Destination address (bits 0–7)
    return canID;
}

// Decode extended CAN ID
void decodeExtendedCANID(struct extCANIDDecoded *myCAN ,uint32_t canID) {
    myCAN->PRIORITY = (canID >> 24) & 0xFF;        // Extract priority (bits 24-29)
    myCAN->BASE_ID = (canID >> 20) & 0x0F;         // Extract Base ID (bits 20-23)
    myCAN->SRC = (canID >> 12) & 0xFF;             // Extract Source Address (bits 12-19)
    myCAN->DEST = (canID >> 4) & 0xFF;             // Extract destination address (bits 4-11)
    myCAN->MSG_NUM = canID & 0x0F;                 // Extract Message Number (bits 0-3)
}

void decodeStandardCANID(struct StandardCANIDDecoded *myCAN, uint32_t canID) {
    myCAN->PRIORITY = (canID >> 8) & 0x0F;        // Extract priority (bits 24-29)
    myCAN->SRC = (canID >> 4) & 0x0F;             // Extract destination address (bits 4-11)
    myCAN->MSG_NUM = canID & 0x0F;                 // Extract Message Number (bits 0-3)
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











