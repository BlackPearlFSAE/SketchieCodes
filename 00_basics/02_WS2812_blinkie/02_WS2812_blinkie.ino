#include <FastLED.h>

#define LED_PIN 12       // WS2812 Data Pin
#define NUM_LEDS 3       // Number of LEDs
#define BRIGHTNESS 75   // LED Brightness (0-255)
#define HUE_STEP 0.5     // Step size for hue transition

CRGB leds[NUM_LEDS];
float hue = 0.0;  // Floating-point hue

void setup() {
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);
}

void loop() {
    smoothRainbow(HUE_STEP, 20);  // Smooth floating-point hue transition
}

// ---- Smooth Floating-Point Hue Function ----
void smoothRainbow(float step, int wait) {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CHSV((int)hue % 256, 255, 255); // Convert float to 8-bit hue
        hue += step; // Increment hue smoothly
        if (hue >= 256.0) hue = 0.0; // Reset hue after full cycle
    }
    FastLED.show();
    delay(wait);
}
