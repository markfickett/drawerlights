#include <Adafruit_NeoPixel.h>

#include <PinChangeInterrupt.h>

#define NEO_PIXEL_PIN 9
#define NUM_LEDS 60

#define PIN_SWITCH_START 0
#define NUM_SWITCHES 8

#define LEDS_PER_SWITCH 7

Adafruit_NeoPixel strip = Adafruit_NeoPixel(
    NUM_LEDS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

volatile boolean changed;

void handleSwitchChange() {
  changed = true;
}

void setup() {
  strip.begin();
  changed = true;
  for(int switchIndex = PIN_SWITCH_START;
      switchIndex < PIN_SWITCH_START + NUM_SWITCHES;
      switchIndex++) {
    pinMode(switchIndex, INPUT_PULLUP);
    attachPCINT(
        digitalPinToPCINT(switchIndex), handleSwitchChange, CHANGE);
  }
}

void loop() {
  if (changed) {
    delay(10); // debounce
    for(int switchIndex = PIN_SWITCH_START;
        switchIndex < PIN_SWITCH_START + NUM_SWITCHES;
        switchIndex++) {
      uint32_t color = digitalRead(switchIndex) == HIGH
          ? strip.Color(255, 255, 80) : strip.Color(0, 0, 80);
      for (int ledIndex = 0; ledIndex < LEDS_PER_SWITCH; ledIndex++) {
        strip.setPixelColor(switchIndex * LEDS_PER_SWITCH + ledIndex, color);
      }
    }
    strip.show();
    changed = false;
  }
}
