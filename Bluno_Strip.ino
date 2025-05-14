#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <FastLED.h>
#include "esp_gap_ble_api.h"

#define LED_PIN     5
#define NUM_LEDS    72
#define BRIGHTNESS  100
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_RX "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_TX "beb5483f-36e1-4688-b7f5-ea07361b26a8"

CRGB leds[NUM_LEDS];
BLECharacteristic* pTxCharacteristic;
bool deviceConnected = false;
esp_bd_addr_t lastPeerAddress;

unsigned long lastActivityTime = 0;
int pulseValue = 0;
int pulseDirection = 1;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    delay(500);
    BLEDevice::startAdvertising();
  }
};

void gapCallback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
  if (event == ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT) {
    memcpy(lastPeerAddress, param->update_conn_params.bda, sizeof(esp_bd_addr_t));

    esp_ble_conn_update_params_t conn_params = {
      .bda = {0},
      .min_int = 0x18,
      .max_int = 0x28,
      .latency = 0,
      .timeout = 400
    };
    memcpy(conn_params.bda, lastPeerAddress, sizeof(esp_bd_addr_t));
    esp_ble_gap_update_conn_params(&conn_params);

    esp_ble_gap_set_preferred_phy(
      lastPeerAddress,
      ESP_BLE_GAP_PHY_1M,
      ESP_BLE_GAP_PHY_2M,
      ESP_BLE_GAP_PHY_2M,
      ESP_BLE_GAP_PHY_OPTIONS_NO_PREF
    );
  }
}

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    portENTER_CRITICAL(&mux);
    lastActivityTime = millis();
    portEXIT_CRITICAL(&mux);
    const uint8_t* data = pCharacteristic->getData();
    size_t len = pCharacteristic->getLength();
    int ledCount = len / 3;
    if (ledCount == 0) return;

    for (int i = 0; i < NUM_LEDS; i++) {
      int idx = (i * ledCount) / NUM_LEDS;
      if (idx >= ledCount) idx = ledCount - 1;
      leds[i] = CRGB(data[idx * 3], data[idx * 3 + 1], data[idx * 3 + 2]);
    }

    FastLED.show();

    uint8_t doneByte[1] = { 0x01 };
    pTxCharacteristic->setValue(doneByte, 1);
    pTxCharacteristic->notify();
  }
};

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear(); FastLED.show();

  BLEDevice::init("Bluno strip");
  BLEDevice::setMTU(247);
  esp_ble_gap_register_callback(gapCallback);

  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE_NR
  );
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pTxCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void loop() {
  unsigned long diff;
  portENTER_CRITICAL(&mux);
  diff = millis() - lastActivityTime;
  portEXIT_CRITICAL(&mux);
  if (diff >= 10000) {
    if (diff <= 70000 ) {
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
