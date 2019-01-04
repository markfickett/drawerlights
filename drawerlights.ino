#include <Adafruit_NeoPixel.h>

#define NEO_PIXEL_PIN A2
#define NUM_LEDS 60

#define NUM_DRAWERS 8
#define LEDS_PER_DRAWER 4

#define PIN_SWITCH_START 2

#define DEBOUNCE_MS 10
#define TIMEOUT_MS 120000

#define PIN_PHOTO_SENSE A0
#define PIN_PHOTO_SENSE_SUPPLY A1
#define LIGHT_THRESHOLD 500

Adafruit_NeoPixel strip = Adafruit_NeoPixel(
    NUM_LEDS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

class Drawer {
  private:
    uint16_t start;
    int switchPin;
    boolean ledsOn;
    boolean lastDrawerOpen;
    unsigned long lastDrawerChangeMillis;

  public:
    Drawer(uint16_t i_start, int i_switchPin):
        start(i_start),
        switchPin(i_switchPin),
        ledsOn(false),
        lastDrawerOpen(false),
        lastDrawerChangeMillis(millis()) {
    }

    void setup() {
      pinMode(switchPin, INPUT_PULLUP);
    }

    boolean update(boolean bright) {
      boolean ledsChanged = false;
      // pullup + NC switch pressed by drawer
      // therefore HIGH means the drawer is closed means LEDs off
      boolean drawerOpen = digitalRead(switchPin) == LOW;
      // millis() will roll over every 49.7 days (with 32-bit unsigned long).
      // Thus if a drawer stands open for a month and a half, the timeout
      // will reset temporarily.
      unsigned long t = millis();
      unsigned long millisSinceDrawerChange = t - lastDrawerChangeMillis;

      if (drawerOpen != lastDrawerOpen) {
        if (millisSinceDrawerChange > DEBOUNCE_MS && ledsOn != drawerOpen) {
          ledsOn = drawerOpen;
          ledsChanged = true;
        }
        lastDrawerChangeMillis = t;
        lastDrawerOpen = drawerOpen;
      } else if (ledsOn && millisSinceDrawerChange > TIMEOUT_MS) {
        ledsOn = false;
        ledsChanged = true;
      }

      if (ledsChanged) {
        uint32_t color = ledsOn
            ? (bright ? strip.Color(255, 255, 0) : strip.Color(100, 50, 0))
            : strip.Color(0, 0, 0);
        for (uint16_t i = start; i < start + LEDS_PER_DRAWER; i++) {
          strip.setPixelColor(i, color);
        }
      }
      return ledsChanged;
    }
};

Drawer* drawers[NUM_DRAWERS];

void setup() {
  strip.begin();
  for(int i = 0; i < NUM_DRAWERS; i++) {
    drawers[i] = new Drawer(i * LEDS_PER_DRAWER, PIN_SWITCH_START + i);
    drawers[i]->setup();
  }
  pinMode(PIN_PHOTO_SENSE_SUPPLY, OUTPUT);
  digitalWrite(PIN_PHOTO_SENSE_SUPPLY, LOW);
  pinMode(PIN_PHOTO_SENSE, INPUT);
}

void loop() {
  digitalWrite(PIN_PHOTO_SENSE_SUPPLY, HIGH);
  boolean bright = analogRead(PIN_PHOTO_SENSE) > LIGHT_THRESHOLD;
  digitalWrite(PIN_PHOTO_SENSE_SUPPLY, LOW);

  boolean ledsChanged = false;
  for(int i = 0; i < NUM_DRAWERS; i++) {
    ledsChanged |= drawers[i]->update(bright);
  }
  if (ledsChanged) {
    strip.show();
  }
}
