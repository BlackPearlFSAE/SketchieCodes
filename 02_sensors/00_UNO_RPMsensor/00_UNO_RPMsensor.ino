// For AVR boards mainly Arduino UNO rev3
// Arduino UNO rev3 supports library timer one which will make timerinterrupt usage simple
// Input reading and

// HallEffect sensor used is ... , change ENCODER_N according to the ABS tone ring teeth

// For ESP32 and other Espressif microcontrollers only.
// delete timer one codeto use the one from #include <esp32-hal-timer.h>
// The sensor does not works with 3.3V logic families , therefore logic

// Memory usage:
// On ESP32S3devkitc-1: 22% of program storage space and 5% of dynamic memory occupied

#include <Arduino.h>
#include <TimerOne.h>
#include <Ticker.h>
#include <math.h>

  #define ENCODER_PIN 3
  #define ENCODER_N 50 // encoding resolution
  #define MOTORPIN 5

// RPM measurement Variables
unsigned int Motor_RPM = 0;
unsigned long T = 100; // 100 ms
bool direction; // 1: CW , 0:CCW
// ISR shared variable
volatile int counter = 0; // pulse count as float to prevent zero
volatile bool startCalculate = 0;


/*-----------------------ISR*/
void ISR_COUNT(){
  counter++;
}
void ISRreset() {
  startCalculate = true;
}

/*===================================*/
void setup() {
  Serial.begin(115200);
  pinMode(ENCODER_PIN, INPUT); // The module has internal Pull up Resistor
  pinMode(MOTORPIN,OUTPUT);
  
  // ------ interupt on Halleffect sensor
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN),ISR_COUNT, FALLING); // interrupt on falling edge
  
  // ------ Set up 100ms timer
+  Timer1.initialize(100000); 
  Timer1.attachInterrupt(ISRreset);
  
  /* When Object blocks (Open Switch) The signal is HIGH , 
    so RISING edge of pulse will fires ISR
    pull up , so when change should be increment on falling edge */
}

unsigned long lasttime = 0;
void loop() {

  if(startCalculate){
    noInterrupts();
    Serial.println(counter);
    Motor_RPM = (30000)/T * ((float)counter/ ENCODER_N);
    Serial.print("RPM: "); Serial.println(Motor_RPM);
    counter = 0;
    startCalculate = false;
    interrupts();
  }

  /* Motor Driving Test Section */
  digitalWrite(MOTORPIN,HIGH);
}


// A function yo better the tachometer output , deflauctuation , and increase resolution
// averaging a count pulse , from what I obserbed within 100ms , the value range from 7-10 , 8 appear most , 
// not sure if I should round the averaged result or keep it there , keeping it there makes floating result finder in resolution
// but does that match with reality where RPM is suppose to be integer
// how much time do i need to sample the data to average , 50 ms ?? , 90 ms ??


// for this type of sensor , averging the rpm can't help , since the value are too off
// Averagin the counter , and floor down seems good , as flooring down counter the bouncy nature of optical sensor

// using mode , as counter is discrete point , is also not bad , like using search for most , 
// but that seems memory and time expensive

//   // Function to average the rpm for every 99 ms (should be less than 100ms)
//   // Algorithm used . moving average, exponential average , possible not using loop

// Averaging algorithm

// 1. Low Pass Filter (LPF)


// 2. exponential moving average (EMA)



