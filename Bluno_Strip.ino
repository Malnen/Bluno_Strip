#include <FastLED.h>

#define LED_PIN       5       // Data pin for LED strip
#define NUM_LEDS      72      // Number of LEDs
#define BRIGHTNESS    255     // Brightness level (0–255)
#define BUILTIN_LED   15      // Built-in LED pin (adjust if needed)

CRGB leds[NUM_LEDS];         // LED strip array

void setup() {
  Serial.begin(115200);
  pinMode(BUILTIN_LED, OUTPUT);

  // Initialize LED strip
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  Serial.println("🚀 Setup complete. Starting LED sequence...");
}

void loop() {
  Serial.println("🎨 Starting RED wipe...");
  digitalWrite(BUILTIN_LED, HIGH);  // Turn ON built-in LED

  // Red color wipe
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Red;
  }

  FastLED.show();
  Serial.println("🛑 Pause after RED wipe");
  delay(100);

  Serial.println("💡 Turning off LEDs...");
  digitalWrite(BUILTIN_LED, LOW);  // Turn OFF built-in LED

  // Turn all LEDs off
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  delay(100);
}
