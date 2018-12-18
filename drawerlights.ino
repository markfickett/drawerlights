#include <Adafruit_NeoPixel.h>
// https://github.com/NicoHood/PinChangeInterrupt
#include <PinChangeInterrupt.h>

#define NEO_PIXEL_PIN 9
#define NUM_LEDS 60

#define NUM_DRAWERS 8
#define LEDS_PER_DRAWER 4

#define PIN_SWITCH_START 0

Adafruit_NeoPixel strip = Adafruit_NeoPixel(
    NUM_LEDS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

volatile boolean changed;

void handleSwitchChange() {
  changed = true;
}

class Drawer {
  private:
    uint16_t start;
    int switchPin;

    void set(boolean on) {
      uint32_t color = on ? strip.Color(255, 255, 0) : strip.Color(0, 0, 0);
      for (uint16_t i = start; i < start + LEDS_PER_DRAWER; i++) {
        strip.setPixelColor(i, color);
      }
    }
  public:
    // for array initialization; essentially unused
    Drawer(): start(0), switchPin(0) {}

    Drawer(uint16_t i_start, int i_switchPin):
        start(i_start), switchPin(i_switchPin) {
      pinMode(switchPin, INPUT_PULLUP);
      attachPCINT(
        digitalPinToPCINT(switchPin), handleSwitchChange, CHANGE);
    }

    void updateFromSwitch() {
      // pullup + NC switch pressed by drawer
      // therefore HIGH means the drawer is closed means LEDs off
      set(digitalRead(switchPin) == LOW);
    }
};

Drawer drawers[NUM_DRAWERS];

void setup() {
  strip.begin();
  for(int i = 0; i < NUM_DRAWERS; i++) {
    drawers[i] = Drawer(i * LEDS_PER_DRAWER, PIN_SWITCH_START + i);
  }
  changed = true;
}

void loop() {
  if (changed) {
    delay(10); // debounce
    for(int i = 0; i < NUM_DRAWERS; i++) {
      drawers[i].updateFromSwitch();
    }
    strip.show();
    changed = false;
  }
}
