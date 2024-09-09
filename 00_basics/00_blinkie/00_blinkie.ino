// For ESP32 and other Espressif microcontrollers only.
//
// Memory usage:
// On ESP32-WROOM-32: 20% of program storage space and 6% of dynamic memory occupied
//
// Some common issues and fixes:
//
// don't forget to remove or disable brltty
// REF: https://forum.arduino.cc/t/port-not-showing-up-ch340-in-ubuntu-solution/1176862/4
//
// Run this if you've just started first time: pip3 install pyserial
// REF: https://forum.arduino.cc/t/modulenotfounderror-no-module-named-serial/847838/2
//
// Unable to flash ESP32: the port does not exist
// REF: https://stackoverflow.com/questions/73923341/unable-to-flash-esp32-the-port-doesnt-exist
//
// Tutorial example: https://randomnerdtutorials.com/esp32-pwm-arduino-ide/

#define PIN_LED 2
#define PWM_CHANNEL 0
#define PWM_FREQUENCY 5000
#define PWM_RESOLUTION 8

void setup() {
  // configure LED PWM
  ledcAttach(PIN_LED, PWM_FREQUENCY, PWM_RESOLUTION);

  // if you want to attach a specific channel, use the following instead
  // ledcAttachChannel(PIN_LED, PWM_FREQUENCY, PWM_RESOLUTION, PWM_CHANNEL);
}

void loop() {
  // Fade up
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle+=10) {
    ledcWrite(PIN_LED, dutyCycle);
    delay(10); // Adjust delay for desired fade speed
  }

  delay(20);

  // Fade down
  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle-=10) {
    ledcWrite(PIN_LED, dutyCycle);
    delay(10);
  }
}
