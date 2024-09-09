# SketchieCodes
Collection of Arduino sketches, applicable for Arduino, ESP32, Teensy micro-controllers used in Black Pearl Racing Team.

## Code contribution guidelines

*We accept the codes that are not in code convention guidelines.*

### Code structure

Here is a basic structure of sketch structure, aimed at maximum reusability:

```c++
// COMMENTS
// What boards does the code support?
// Why boards were used for testing code?
// Any issues we should be aware of?
// Are there any prerequisites required to run the code? Libraries? Setup?
// Do you need to credit any external contributors for that part of code?

// CODE
// #pre-processors (#include and #define)
// constants
// interrupt routines declaration (if any)
// setup() function
// loop() funtion
// interrupt routines definition (if any)
```

## Arduino IDE v2 Setup Documentation

### Linux systems

We recommend using AppImage for Linux based system. Have them in `~/Applications` directory. Desktop integration on major distros can be achieved with [AppImageLauncher](https://github.com/TheAssassin/AppImageLauncher).

When you are doing the initial setup, you might encounter some issues regarding ports on Linux. Here's a basic fix without much of explanation:
```bash
# one time only
sudo apt remove brltty
pip install pyserial

sudo adduser $USER dialout # one time only
sudo chmod a+rw /dev/ttyUSB0 # on whatever TTY port you got issues with
sudo chown username /dev/ttyUSB0 # even solid fix alternative
```

References:
1. https://forum.arduino.cc/t/port-not-showing-up-ch340-in-ubuntu-solution/1176862/4
2. https://forum.arduino.cc/t/modulenotfounderror-no-module-named-serial/847838/2
3. https://randomnerdtutorials.com/esp32-pwm-arduino-ide/
