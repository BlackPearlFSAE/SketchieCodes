// Black board yss

#include <driver/twai.h>
// Variables
unsigned long logCount = 0;
unsigned long lastStatTime = 0;
unsigned long lastLogTime = 0;

// Battery data structure
struct BatteryData {
  bool chargingMode;
  uint16_t balancingCells;
  float moduleVoltage;
  float cellVoltageDiff;
  float temp1;
  float temp2;
} batteryData;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println("ESP32 Battery Monitoring System Initializing");

  
  // Initialize TWAI (CAN) driver
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)TWAI_TX_PIN, (gpio_num_t)TWAI_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  
  // Install TWAI driver
  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    Serial.println("TWAI Driver installed successfully");
  } else {
    Serial.println("Failed to install TWAI driver");
    return;
  }
  
  // Start TWAI driver
  if (twai_start() == ESP_OK) {
    Serial.println("TWAI Driver started successfully");
  } else {
    Serial.println("Failed to start TWAI driver");
    return;
  }

  
  Serial.println("TWAI/CAN Initialized at 500Kbps");
  Serial.println("Battery monitoring system ready!");
}

void loop() {
  // Check for CAN messages
  twai_message_t message;
  if (twai_receive(&message, pdMS_TO_TICKS(0)) == ESP_OK) {
    // Process received message
    processTWAIMessage(message);
  }
  
  // Log data periodically
  unsigned long currentMillis = millis();
  if (currentMillis - lastLogTime >= LOG_INTERVAL_MS && batteryData.dataUpdated) {
    lastLogTime = currentMillis;
    logBatteryData();
    batteryData.dataUpdated = false;  // Reset flag until next update
  }
  
  // Print stats periodically
  if (currentMillis - lastStatTime >= 10000) { // Every 10 seconds
    lastStatTime = currentMillis;
    Serial.printf("Data points logged: %lu\n", logCount);
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
  }
}

void processTWAIMessage(twai_message_t &message) {
  // Process only messages from the BMU
  if (message.identifier == BMU_STATUS_ID) {
    // Byte 0: BMU Enter Charging Mode
    batteryData.chargingMode = (message.data[0] == 1);
    
    // Byte 1-2: Cells in Balancing (10-bit representation)
    batteryData.balancingCells = (message.data[1] << 8) | message.data[2]; // Combine into 16-bit
    
    // Byte 3-4: Module Voltage (0.00-42.00V with 0.02V resolution)
    batteryData.moduleVoltage = message.data[3] * 0.02; // Factor 0.02
    
    // Byte 5: Cell voltage difference (0-0.2V with 0.1V resolution)
    batteryData.cellVoltageDiff = message.data[5] * 0.1; // Factor 0.1
    
    // Byte 6: Temperature sensor 1 (already in Celsius)
    batteryData.temp1 = (float)message.data[6]*6;
    
    // Byte 7: Temperature sensor 2 (already in Celsius)
    batteryData.temp2 = (float)message.data[7]*6;
    

  } else {
    // For any other CAN IDs, print the raw data (for debugging)
    Serial.print("Other CAN ID: 0x");
    Serial.print(message.identifier, HEX);
    Serial.print(" Data: ");
    for (int i = 0; i < message.data_length_code; i++) {
      Serial.print(message.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
}
