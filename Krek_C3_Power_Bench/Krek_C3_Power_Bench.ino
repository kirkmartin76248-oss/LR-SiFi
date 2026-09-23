/*
  KREK LABS - ESP32-C3 NODE POWER BENCH TEST

  Pin assignment:
    GPIO0 = Analog Sensor 1
    GPIO1 = Analog Sensor 2
    GPIO2 = Battery monitor
    GPIO3 = Analog Sensor 3
    GPIO4 = Analog Sensor 4
    GPIO6 = TPS22929DBVR ON/EN
    GPIO8 = Onboard LED

  Battery monitor:
    BAT+ -> 1M -> GPIO2 -> 1M -> GND
    GPIO2 -> 100nF -> GND

  Bench:
    3.6V supply
    ~30mA simulated sensor load

  SENSOR_SETTLE_MS is the only intentional delay in the
  active measurement cycle. ESP-NOW LR is intentionally
  not enabled until C3/C6 compatibility is verified.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "esp_sleep.h"

constexpr uint8_t SENSOR1_PIN = 0;
constexpr uint8_t SENSOR2_PIN = 1;
constexpr uint8_t BATTERY_PIN = 2;
constexpr uint8_t SENSOR3_PIN = 3;
constexpr uint8_t SENSOR4_PIN = 4;

constexpr uint8_t SENSOR_POWER_PIN = 6;
constexpr uint8_t LED_PIN = 8;

// Change this one value to test sensor stabilization time.
constexpr uint32_t SENSOR_SETTLE_MS = 100;

// Short bench interval for repeated Joulescope captures.
constexpr uint32_t BENCH_SLEEP_SECONDS = 10;

constexpr uint8_t ESP_NOW_CHANNEL = 1;

uint8_t broadcastAddress[] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

struct KrekPacket {
  uint32_t sequence;
  uint16_t sensor1;
  uint16_t sensor2;
  uint16_t sensor3;
  uint16_t sensor4;
  uint16_t batteryADC;
};

uint32_t sequenceNumber = 0;

void logEvent(const char *event)
{
  Serial.print(millis());
  Serial.print(" ms  ");
  Serial.println(event);
}

bool setupESPNow()
{
  logEvent("ESP-NOW INIT");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  esp_err_t result = esp_wifi_set_channel(
    ESP_NOW_CHANNEL,
    WIFI_SECOND_CHAN_NONE
  );

  if (result != ESP_OK) {
    Serial.print("ERROR: channel setup failed: ");
    Serial.println(result);
    return false;
  }

  result = esp_now_init();

  if (result != ESP_OK) {
    Serial.print("ERROR: ESP-NOW init failed: ");
    Serial.println(result);
    return false;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = ESP_NOW_CHANNEL;
  peerInfo.encrypt = false;

  result = esp_now_add_peer(&peerInfo);

  if (result != ESP_OK && result != ESP_ERR_ESPNOW_EXIST) {
    Serial.print("ERROR: peer setup failed: ");
    Serial.println(result);
    return false;
  }

  logEvent("ESP-NOW READY");
  return true;
}

void transmitPacket(
  uint16_t sensor1,
  uint16_t sensor2,
  uint16_t sensor3,
  uint16_t sensor4,
  uint16_t batteryADC
)
{
  KrekPacket packet;

  packet.sequence = sequenceNumber;
  packet.sensor1 = sensor1;
  packet.sensor2 = sensor2;
  packet.sensor3 = sensor3;
  packet.sensor4 = sensor4;
  packet.batteryADC = batteryADC;

  logEvent("ESP-NOW TX START");

  esp_err_t result = esp_now_send(
    broadcastAddress,
    reinterpret_cast<uint8_t *>(&packet),
    sizeof(packet)
  );

  if (result == ESP_OK) {
    logEvent("ESP-NOW TX ACCEPTED");
  } else {
    Serial.print("ESP-NOW TX ERROR: ");
    Serial.println(result);
  }
}

void setup()
{
  Serial.begin(115200);

  // Development-only serial startup delay.
  // Remove before final production power measurements.
  delay(10);

  Serial.println();
  Serial.println("================================");
  Serial.println("KREK C3 NODE POWER BENCH TEST");
  Serial.println("================================");

  pinMode(SENSOR_POWER_PIN, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN, LOW);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  analogReadResolution(12);

  analogSetPinAttenuation(SENSOR1_PIN, ADC_11db);
  analogSetPinAttenuation(SENSOR2_PIN, ADC_11db);
  analogSetPinAttenuation(SENSOR3_PIN, ADC_11db);
  analogSetPinAttenuation(SENSOR4_PIN, ADC_11db);
  analogSetPinAttenuation(BATTERY_PIN, ADC_11db);

  logEvent("WAKE");

  digitalWrite(LED_PIN, HIGH);
  logEvent("LED ON");

  digitalWrite(SENSOR_POWER_PIN, HIGH);
  logEvent("SENSOR POWER ON");

  if (SENSOR_SETTLE_MS > 0) {
    Serial.print("Sensor settle: ");
    Serial.print(SENSOR_SETTLE_MS);
    Serial.println(" ms");
    delay(SENSOR_SETTLE_MS);
  }

  logEvent("READ SENSOR 1");
  uint16_t sensor1 = analogRead(SENSOR1_PIN);

  logEvent("READ SENSOR 2");
  uint16_t sensor2 = analogRead(SENSOR2_PIN);

  logEvent("READ SENSOR 3");
  uint16_t sensor3 = analogRead(SENSOR3_PIN);

  logEvent("READ SENSOR 4");
  uint16_t sensor4 = analogRead(SENSOR4_PIN);

  logEvent("READ BATTERY");
  uint16_t batteryADC = analogRead(BATTERY_PIN);

  uint32_t batteryMilliVolts = analogReadMilliVolts(BATTERY_PIN);
  float batteryVoltage = (batteryMilliVolts * 2.0f) / 1000.0f;

  digitalWrite(SENSOR_POWER_PIN, LOW);
  logEvent("SENSOR POWER OFF");

  Serial.print("S1 ADC = "); Serial.println(sensor1);
  Serial.print("S2 ADC = "); Serial.println(sensor2);
  Serial.print("S3 ADC = "); Serial.println(sensor3);
  Serial.print("S4 ADC = "); Serial.println(sensor4);
  Serial.print("Battery ADC = "); Serial.println(batteryADC);
  Serial.print("Battery = "); Serial.print(batteryVoltage, 3);
  Serial.println(" V");

  bool radioReady = setupESPNow();

  if (radioReady) {
    transmitPacket(
      sensor1,
      sensor2,
      sensor3,
      sensor4,
      batteryADC
    );
  } else {
    logEvent("ESP-NOW NOT READY");
  }

  sequenceNumber++;

  digitalWrite(LED_PIN, LOW);
  logEvent("LED OFF");

  digitalWrite(SENSOR_POWER_PIN, LOW);

  logEvent("DEEP SLEEP");

  Serial.flush();

  esp_sleep_enable_timer_wakeup(
    (uint64_t)BENCH_SLEEP_SECONDS * 1000000ULL
  );

  esp_deep_sleep_start();
}

void loop()
{
  // Never reached.
}
