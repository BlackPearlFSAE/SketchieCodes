// For ESP32 boards
// Espressif32 architecture has addittional Hardware Serial port, Serial1 and Serial2

// The code serves as an Example to Serialize Struct data to array, and vice versa
// Show casing simple UART communication in 2Mbps

// Memory usage:
// On ESP32S3devkitc-1: 21% of program storage space and 5% of dynamic memory occupied


#include <Arduino.h>
#include <esp32-hal-uart.h>
#include <HardwareSerial.h>
#include <driver/gpio.h>

#define UART_TX GPIO_NUM_38
#define UART_RX GPIO_NUM_37

// CAN Message Structure
struct MessageFrame {
    uint32_t id;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3; 
};

// Serialize message to byte array
void serialize(uint8_t* buffer, MessageFrame *my_msg) {
  uint32_t id = my_msg->id;
  byte data1= my_msg->data1;
  byte data2= my_msg->data2;
  byte data3= my_msg->data3;

  // Copy memory block from id to buffer 1st block of memory by specified size
    memcpy(buffer, &id, sizeof(id)); // ID should have the size of 4 byte , so 1st four array block will be 
    memcpy(buffer + sizeof(id), &data1, sizeof(data1));
    memcpy(buffer + sizeof(id) + sizeof(data1), &data2, sizeof(data2));
    memcpy(buffer + sizeof(id) + sizeof(data1) + sizeof(data2), &data3, sizeof(data3));
}

// Deserialize byte array to message
static MessageFrame deserialize(const uint8_t* buffer) {
    MessageFrame msg;
    memcpy(&msg.id, buffer, sizeof(msg.id));
    memcpy(&msg.data1, buffer + sizeof(msg.id), sizeof(msg.data1));
    memcpy(&msg.data2, buffer + sizeof(msg.id) + sizeof(msg.data1), sizeof(msg.data2));
    memcpy(&msg.data3, buffer + sizeof(msg.id) + sizeof(msg.data1) + sizeof(msg.data2), sizeof(msg.data3));
    return msg;
}

// UART Transmission
void SendtoUART(uint8_t *txBuf, unsigned int len){
    Serial1.write(txBuf , len);
}

// UART Reception
void receiveMessageUART(uint8_t* rxBuf, unsigned int len) {
    if (Serial1.available() > 0 ) {
      // Should I just use read until , and add some delimeter 
        Serial1.readBytes(rxBuf, len);
        // Because in this case we need to match to exact size, other wise we wil get data from other frame
    } 
}
// ===========================================================================================
/*-------------------------------*/
  struct MessageFrame Tx_msg;
  struct MessageFrame Rx_msg;
  /*---------- TX Buffer ---------- */
  uint8_t tx_buf[sizeof(MessageFrame)] = {0};
  /*---------- RX Buffer ---------- */
  uint8_t rx_buf[sizeof(MessageFrame)] = {0};
// ===========================================================================================
void setup(){
  Serial.begin(115200);
  // Configure UART pin by ESP32 hal
  Serial1.begin(2000000); // 2Mbps , actualt maximum seems to be 5Mbps , but this is good enough
  Serial1.setPins(UART_RX, UART_TX);

  // Initialize Struct
  Rx_msg.id = 0xFFFFFFFF;
  Rx_msg.data1 = 1;
  Rx_msg.data2 = 1;
  Rx_msg.data3 = 1;

  Tx_msg.id = 0xEEEEEEEE;
  Tx_msg.data1 = 1;
  Tx_msg.data2 = 1;
  Tx_msg.data3 = 1;
}

bool receiveMode = 0;

void loop(){
  
  // Receive UART
  if(receiveMode){
    Serial.println("-------RX UART");
    // Check Receive Array
    receiveMessageUART(rx_buf, sizeof(rx_buf));
    for(int i = 0; i < sizeof(rx_buf) - 1; i++ ){
      Serial.printf("Index %d : ", i);
      Serial.println(rx_buf[i]);
    }
    // Deserialize back to struct
    struct MessageFrame temp;
    temp = deserialize(rx_buf);
    /*
      Code to use struct above
    */
  } 

  // Sent UART 
  else {
    Serial.println("-------TX UART");
    serialize(tx_buf, &Tx_msg);

    // Check Serialize
    for(int i = 0; i < sizeof(tx_buf) - 1; i++ ){
      Serial.printf("Index %d : ", i);
      Serial.println(tx_buf[i]);
    }
    /* Send 2Mbit/s */
    SendtoUART(tx_buf, sizeof(tx_buf));
  }
  vTaskDelay(500/ portTICK_PERIOD_MS);
}