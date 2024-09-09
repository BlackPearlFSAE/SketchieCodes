# SketchieCodes
Collection of Arduino sketches, applicable for Arduino, ESP32, Teensy micro-controllers used in Black Pearl Racing Team.

## Arduino IDE v2 Setup Documentation


### Linux systems

We recommend using AppImage for Linux based system. Have them in `~/Applications` directory. Desktop integration on major distros can be achieved with [AppImageLauncher]([url](https://github.com/TheAssassin/AppImageLauncher)).

When you are doing the initial setup, you might encounter some issues regarding ports on Linux. Here's a basic fix without much of explanation:
```bash
# one time only
sudo apt remove brltty
pip install pyserial

sudo adduser $USER dialout # one time only
sudo chmod a+rw /dev/ttyUSB0 # on whatever TTY port you got issues with
```

References:
1. https://forum.arduino.cc/t/port-not-showing-up-ch340-in-ubuntu-solution/1176862/4
2. https://forum.arduino.cc/t/modulenotfounderror-no-module-named-serial/847838/2
3. https://randomnerdtutorials.com/esp32-pwm-arduino-ide/
