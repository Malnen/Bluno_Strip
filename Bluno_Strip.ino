#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <FastLED.h>
#include "esp_wifi.h"

#define LED_PIN     5
#define NUM_LEDS    72
#define BRIGHTNESS  100
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

const char* WIFI_SSID     = "";
const char* WIFI_PASSWORD = "";
const unsigned int UDP_PORT = 4210;

const char* DEVICE_ID = "b4e91b7e";

WiFiUDP udp;
CRGB leds[NUM_LEDS];

unsigned long lastActivityTime = 0;
int pulseValue = 0;
int pulseDirection = 1;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  Serial.println("Connected!");
  Serial.printf("ESP32 IP: %s\n", WiFi.localIP().toString().c_str());

  esp_wifi_set_ps(WIFI_PS_NONE);

  if (MDNS.begin(DEVICE_ID)) {
    MDNS.addService("rgbapp", "udp", UDP_PORT);
    MDNS.addServiceTxt("rgbapp", "udp", "name", "Bluno Strip");
    MDNS.addServiceTxt("rgbapp", "udp", "id", DEVICE_ID);
    Serial.printf("mDNS started: %s.local\n", DEVICE_ID);
  } else {
    Serial.println("mDNS setup failed");
  }

  udp.begin(UDP_PORT);
  Serial.printf("UDP server listening on port %u\n", UDP_PORT);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  lastActivityTime = millis();
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    IPAddress srcIP = udp.remoteIP();
    unsigned int srcPort = udp.remotePort();

    uint8_t buffer[NUM_LEDS * 3];
    int len = udp.read(buffer, sizeof(buffer));
    if (len <= 0) return;

    portENTER_CRITICAL(&mux);
    lastActivityTime = millis();
    portEXIT_CRITICAL(&mux);

    Serial.printf("📥 Received %d bytes from %s:%u\n", len, srcIP.toString().c_str(), srcPort);

    int ledCount = len / 3;
    if (ledCount > NUM_LEDS) ledCount = NUM_LEDS;

    for (int i = 0; i < NUM_LEDS; i++) {
      int idx = (i * ledCount) / NUM_LEDS;
      if (idx >= ledCount) idx = ledCount - 1;
      leds[i] = CRGB(buffer[idx * 3], buffer[idx * 3 + 1], buffer[idx * 3 + 2]);
    }

    FastLED.show();

    uint8_t doneByte[1] = { 0x01 };
    udp.beginPacket(srcIP, srcPort);
    udp.write(doneByte, 1);
    udp.endPacket();

    Serial.println("✅ Frame applied, sent DONE (0x01)");
  }

  unsigned long diff;
  portENTER_CRITICAL(&mux);
  diff = millis() - lastActivityTime;
  portEXIT_CRITICAL(&mux);

  if (diff >= 10000) {
    if (diff <= 70000) {
      slowPulse();
    } else {
      disabled();
    }
    delay(25);
  }
}

void slowPulse() {
  pulseValue += pulseDirection * 5;
  if (pulseValue <= 0 || pulseValue >= 255) {
    pulseDirection = -pulseDirection;
  }

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(pulseValue, pulseValue, pulseValue);
  }

  FastLED.show();
}

void disabled() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(0, 0, 0);
  }

  FastLED.show();
}
