// For AVR boards mainly Arduino UNO rev3, or any that supports arduino framework
// AVR family board is good for reading analog sensors, and its 5V TTL logic family is highly compatible with most sensors
// UNO rev3 provide approprioate resolution for that 

// Linear potentiometer used is KPM18-50mm , change potdist according to the stroke sensors used

// For ESP32 and other Espressif microcontrollers only.
// Change aref = 5.0 to aref = 3.3 , and change pwmres = 1023 to pwmres = 4095

// Memory usage:
// On ESP32S3devkitc-1: 22% of program storage space and 5% of dynamic memory occupied

#define potpin A0
float distance = 0.0;
float potvolts = 0.0;
float aref = 3.3; // 5V ADC
int pwmres = 1023; // 10 bit ADC resolution
uint8_t potdist = 52.91; // Maximum stroke distance recalibrated with vernier 

void setup() {
  Serial.begin(9600);
}

unsigned long lastTime = 0;
void loop() {
  // Conversion from ADC value back to its sensor reading voltage
  potvolts = analogRead(potpin) * (aref/pwmres) ;

  // Conversion of sensor reading voltage KPM18-50mm distance , total distance is 52.91 mm
  distance = potvolts * (52.91/aref);

  if(millis()-lastTime >= 100){
    // Serial.println(analogRead(potpin));
    // Serial.println(potvolts);
    Serial.println(distance);
    lastTime = millis();
  }  
}
