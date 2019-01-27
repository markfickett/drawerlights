#include <Adafruit_NeoPixel.h>

// Note analog output pins won't drive NeoPixels.
#define NEO_PIXEL_PIN 0
#define NUM_LEDS 60

#define NUM_DRAWERS 8
#define LEDS_PER_DRAWER 7

// Digital input pin for the first drawer-sensing switch.
// NUM_DRAWERS sequential pins are used starting with this one.
#define PIN_SWITCH_START 2

// Overall switch sensing and LED updating interval.
#define UPDATE_INTERVAL_MS 10
#define DEBOUNCE_MS 10
// LEDs automatically turn off after this long.
#define TIMEOUT_MS 120000

#define PIN_PHOTO_SENSE A0
#define PIN_PHOTO_SENSE_SUPPLY A1
// Smaller light-sensor numbers are brighter. Values below this threshold are
// considered "bright". Approximately:
//    500   well lit nighttime room
//    900   shadow in a lit nighttime room
//    1000  very dark (light from laptop screen)
#define LIGHT_THRESHOLD 1000

Adafruit_NeoPixel strip = Adafruit_NeoPixel(
    NUM_LEDS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);
const uint32_t COLOR_OFF = strip.Color(0, 0, 0);

class Drawer {
  private:
    /** Index of first LED used by this drawer. */
    uint16_t start;

    /** Digital input pin to sense whether the drawer's open. */
    int switchPin;

    /**
     * Should the LEDs currently be on? This allows decoupling LED state from
     * sensor inputs (for debouncing and timing out).
     */
    boolean ledsOn;

    /** Parameters from the last update cycle, to avoid extra LED updates. */
    boolean lastDrawerOpen;
    unsigned long lastDrawerChangeMillis;
    uint32_t lastColor;

  public:
    Drawer(uint16_t i_start, int i_switchPin):
        start(i_start),
        switchPin(i_switchPin),
        ledsOn(false),
        lastDrawerOpen(false),
        lastDrawerChangeMillis(millis() - (DEBOUNCE_MS + 1)),
        lastColor(COLOR_OFF) {
    }

    void setup() {
      pinMode(switchPin, INPUT_PULLUP);
    }

    boolean update(boolean bright) {
      // pullup + NC switch pressed by drawer
      // therefore HIGH means the drawer is closed means LEDs off
      boolean drawerOpen = digitalRead(switchPin) == LOW;
      return update(bright, drawerOpen);
    }

    boolean update(boolean bright, boolean drawerOpen) {
      // millis() will roll over every 49.7 days (with 32-bit unsigned long).
      // Thus if a drawer stands open for a month and a half, the timeout
      // will reset temporarily.
      unsigned long t = millis();
      unsigned long millisSinceDrawerChange = t - lastDrawerChangeMillis;

      if (drawerOpen != lastDrawerOpen) {
        if (millisSinceDrawerChange > DEBOUNCE_MS && ledsOn != drawerOpen) {
          ledsOn = drawerOpen;
        }
        lastDrawerChangeMillis = t;
        lastDrawerOpen = drawerOpen;
      } else if (ledsOn && millisSinceDrawerChange > TIMEOUT_MS) {
        ledsOn = false;
      }
      uint32_t color = ledsOn
          ? (bright ? strip.Color(255, 255, 50) : strip.Color(100, 70, 20))
          : COLOR_OFF;

      if (color == lastColor) {
        return false;
      }
      for (uint16_t i = start; i < start + LEDS_PER_DRAWER; i++) {
        strip.setPixelColor(i, color);
      }
      lastColor = color;
      return true;
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

  // Show an initialization pattern: blink each drawer in sequence.
  for(int i = 0; i < NUM_DRAWERS; i++) {
    drawers[i]->update(true /* bright */, true /* drawer open => on */);
    strip.show();
    delay(300);
    drawers[i]->update(true /* bright */, false /* closed => off */);
    strip.show();
  }
}

void loop() {
  digitalWrite(PIN_PHOTO_SENSE_SUPPLY, HIGH);
  boolean bright = analogRead(PIN_PHOTO_SENSE) < LIGHT_THRESHOLD;
  digitalWrite(PIN_PHOTO_SENSE_SUPPLY, LOW);

  boolean anyLedsChanged = false;
  for(int i = 0; i < NUM_DRAWERS; i++) {
    anyLedsChanged |= drawers[i]->update(bright);
  }
  if (anyLedsChanged) {
    strip.show();
  }
  delay(UPDATE_INTERVAL_MS);
}
