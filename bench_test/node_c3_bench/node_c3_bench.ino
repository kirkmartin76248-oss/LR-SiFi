/*
  KREK LABS - WiFi William
  ESP32-C3 NODE BENCH TEST

  Locked bench architecture:
    GPIO0 = Sensor Port 1
    GPIO1 = Sensor Port 2
    GPIO2 = Battery ADC
    GPIO3 = Sensor Port 3
    GPIO4 = Sensor Port 4
    GPIO6 = Sensor load-switch enable

  CPU: 80 MHz
  Radio: OFF during sensor delay/read; ON for ESP-NOW transaction.
  Deep sleep handles final radio shutdown.

  Retry policy: initial telemetry attempt + 3 retries = up to 4 attempts.
  Previous wake duration is retained in RTC and transmitted on the NEXT wake.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <esp32-hal-cpu.h>

#define PORT1_PIN             0
#define PORT2_PIN             1
#define BATTERY_ADC_PIN       2
#define PORT3_PIN             3
#define PORT4_PIN             4
#define SENSOR_POWER_PIN      6

#define SENSOR_POWER_ON       HIGH
#define SENSOR_POWER_OFF      LOW

#define WAKE_INTERVAL_MS      30000UL
#define SENSOR_DELAY_MS       3000UL
#define ACK_TIMEOUT_MS        300UL
#define ESPNOW_CHANNEL        1
#define MAX_RETRIES           3

// Replace with the Hub's actual Wi-Fi MAC before testing.
uint8_t HUB_MAC[] = { 0x24, 0x6F, 0x28, 0x12, 0x34, 0x56 };

RTC_DATA_ATTR uint32_t sequenceNumber = 1;
RTC_DATA_ATTR uint32_t previousWakeDurationMs = 0;

const uint32_t configFingerprint = 0xA73C91E2;

volatile bool ackReceived = false;
volatile uint32_t receivedAckSequence = 0;

uint32_t wakeStartMs = 0;

void logMessage(const char *message) {
  Serial.print("[");
  Serial.print(millis());
  Serial.print(" ms] ");
  Serial.println(message);
}

void printMac(const uint8_t *mac) {
  for (int i = 0; i < 6; ++i) {
    if (i) Serial.print(":");
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
  }
}

void setRadioOff() {
  logMessage("RADIO = OFF");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(5);
}

bool setRadioOn() {
  logMessage("RADIO = ON");
  WiFi.mode(WIFI_STA);
  delay(20);
  WiFi.disconnect();
  delay(5);

  esp_err_t result = esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (result != ESP_OK) {
    Serial.print("[radio] channel set failed: ");
    Serial.println(result);
    return false;
  }
  return true;
}

void onDataReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len < 5) return;
  if (data[0] != 0x02) return;

  memcpy((void *)&receivedAckSequence, data + 1, sizeof(uint32_t));
  ackReceived = true;
}

bool initEspNow() {
  logMessage("INITIALIZING ESP-NOW");

  if (esp_now_init() != ESP_OK) {
    logMessage("ESP-NOW INIT FAILED");
    return false;
  }

  esp_now_register_recv_cb(onDataReceive);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, HUB_MAC, 6);
  peer.channel = ESPNOW_CHANNEL;
  peer.encrypt = false;

  esp_err_t result = esp_now_add_peer(&peer);
  if (result != ESP_OK && result != ESP_ERR_ESPNOW_EXIST) {
    Serial.print("[espnow] add peer failed: ");
    Serial.println(result);
    return false;
  }

  logMessage("ESP-NOW READY");
  return true;
}

uint16_t readMillivolts(uint8_t pin) {
  int raw = analogRead(pin);
  return (uint16_t)((raw * 3300UL) / 4095UL);
}

void readSensors(uint16_t &p1, uint16_t &p2, uint16_t &p3, uint16_t &p4) {
  logMessage("READING SENSOR PORTS");

  p1 = readMillivolts(PORT1_PIN);
  p2 = readMillivolts(PORT2_PIN);
  p3 = readMillivolts(PORT3_PIN);
  p4 = readMillivolts(PORT4_PIN);

  Serial.printf("[%lu ms] PORT 1 = %u mV\n", millis(), p1);
  Serial.printf("[%lu ms] PORT 2 = %u mV\n", millis(), p2);
  Serial.printf("[%lu ms] PORT 3 = %u mV\n", millis(), p3);
  Serial.printf("[%lu ms] PORT 4 = %u mV\n", millis(), p4);
}

uint16_t readBatteryMillivolts() {
  logMessage("READING BATTERY");
  uint16_t adcMv = readMillivolts(BATTERY_ADC_PIN);
  uint16_t batteryMv = adcMv * 2;

  Serial.printf("[%lu ms] BATTERY ADC = %u mV\n", millis(), adcMv);
  Serial.printf("[%lu ms] BATTERY = %u mV\n", millis(), batteryMv);
  return batteryMv;
}

size_t buildTelemetry(uint8_t *packet, uint16_t batteryMv,
                     uint16_t p1, uint16_t p2, uint16_t p3, uint16_t p4) {
  // type(1), sequence(4), fingerprint(4), previous wake(4),
  // battery(2), four ports(8) = 23 bytes.
  size_t i = 0;
  packet[i++] = 0x01;
  memcpy(packet + i, &sequenceNumber, 4); i += 4;
  memcpy(packet + i, &configFingerprint, 4); i += 4;
  memcpy(packet + i, &previousWakeDurationMs, 4); i += 4;
  memcpy(packet + i, &batteryMv, 2); i += 2;
  memcpy(packet + i, &p1, 2); i += 2;
  memcpy(packet + i, &p2, 2); i += 2;
  memcpy(packet + i, &p3, 2); i += 2;
  memcpy(packet + i, &p4, 2); i += 2;
  return i;
}

bool transmitWithRetries(uint16_t batteryMv,
                         uint16_t p1, uint16_t p2,
                         uint16_t p3, uint16_t p4) {
  uint8_t packet[23];
  size_t packetLength = buildTelemetry(packet, batteryMv, p1, p2, p3, p4);

  Serial.printf("[%lu ms] PREVIOUS WAKE = %lu ms\n", millis(), previousWakeDurationMs);
  Serial.printf("[%lu ms] SEQUENCE = %lu\n", millis(), sequenceNumber);

  for (uint8_t retry = 0; retry <= MAX_RETRIES; ++retry) {
    ackReceived = false;
    receivedAckSequence = 0;

    Serial.printf("[%lu ms] TX ATTEMPT %u of %u\n",
                  millis(), retry + 1, MAX_RETRIES + 1);

    esp_err_t result = esp_now_send(HUB_MAC, packet, packetLength);
    if (result != ESP_OK) {
      Serial.printf("[%lu ms] ESP-NOW SEND ERROR = %d\n", millis(), result);
    } else {
      uint32_t start = millis();
      while (!ackReceived && (millis() - start < ACK_TIMEOUT_MS)) {
        delay(1);
      }
    }

    if (ackReceived && receivedAckSequence == sequenceNumber) {
      Serial.printf("[%lu ms] ACK RECEIVED FOR SEQUENCE %lu\n",
                    millis(), receivedAckSequence);
      return true;
    }

    Serial.printf("[%lu ms] ACK TIMEOUT / INVALID ACK\n", millis());

    if (retry < MAX_RETRIES) {
      Serial.printf("[%lu ms] RETRYING...\n", millis());
      delay(10);
    }
  }

  logMessage("COMMUNICATION FAILED AFTER 4 TOTAL ATTEMPTS");
  return false;
}

uint32_t calculateSleepTime(uint32_t wakeDurationMs) {
  if (wakeDurationMs >= WAKE_INTERVAL_MS) {
    logMessage("WAKE TIME >= INTERVAL; USING 1 SECOND SLEEP");
    return 1000UL;
  }
  return WAKE_INTERVAL_MS - wakeDurationMs;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" KREK LABS - NODE C3 BENCH TEST");
  Serial.println("========================================");

  bool cpuOk = setCpuFrequencyMhz(80);
  Serial.printf("[%lu ms] CPU = %u MHz (%s)\n",
                millis(), getCpuFrequencyMhz(), cpuOk ? "set" : "warning");

  // Keep radio off for the entire sensor phase.
  setRadioOff();

  pinMode(PORT1_PIN, INPUT);
  pinMode(PORT2_PIN, INPUT);
  pinMode(PORT3_PIN, INPUT);
  pinMode(PORT4_PIN, INPUT);
  pinMode(BATTERY_ADC_PIN, INPUT);
  pinMode(SENSOR_POWER_PIN, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN, SENSOR_POWER_OFF);

  wakeStartMs = millis();
  logMessage("WAKE TIMER STARTED");
  Serial.printf("[%lu ms] SEQUENCE = %lu\n", millis(), sequenceNumber);
  Serial.printf("[%lu ms] PREVIOUS WAKE = %lu ms\n", millis(), previousWakeDurationMs);

  logMessage("SENSOR POWER = ON");
  digitalWrite(SENSOR_POWER_PIN, SENSOR_POWER_ON);

  Serial.printf("[%lu ms] SENSOR DELAY = %lu ms\n", millis(), SENSOR_DELAY_MS);
  delay(SENSOR_DELAY_MS);
  logMessage("SENSOR DELAY COMPLETE");

  uint16_t p1, p2, p3, p4;
  readSensors(p1, p2, p3, p4);
  uint16_t batteryMv = readBatteryMillivolts();

  logMessage("SENSOR POWER = OFF");
  digitalWrite(SENSOR_POWER_PIN, SENSOR_POWER_OFF);

  if (!setRadioOn() || !initEspNow()) {
    logMessage("COMMUNICATION HARDWARE FAILED");
  } else {
    transmitWithRetries(batteryMv, p1, p2, p3, p4);
  }

  // Entire wake period includes sensor work, radio startup, TX, retries and ACK wait.
  uint32_t currentWakeDurationMs = millis() - wakeStartMs;
  Serial.printf("[%lu ms] TOTAL WAKE DURATION = %lu ms\n",
                millis(), currentWakeDurationMs);

  // Save THIS cycle. It will be reported as PREVIOUS wake time next cycle.
  previousWakeDurationMs = currentWakeDurationMs;
  Serial.printf("[%lu ms] RTC SAVED WAKE = %lu ms\n",
                millis(), previousWakeDurationMs);

  uint32_t sleepMs = calculateSleepTime(currentWakeDurationMs);
  Serial.printf("[%lu ms] CONFIGURED INTERVAL = %lu ms\n",
                millis(), WAKE_INTERVAL_MS);
  Serial.printf("[%lu ms] COMPENSATED SLEEP = %lu ms\n",
                millis(), sleepMs);

  sequenceNumber++;
  Serial.printf("[%lu ms] NEXT SEQUENCE = %lu\n", millis(), sequenceNumber);

  Serial.printf("[%lu ms] DEEP SLEEP\n", millis());
  Serial.flush();

  esp_sleep_enable_timer_wakeup((uint64_t)sleepMs * 1000ULL);
  esp_deep_sleep_start();
}

void loop() {}
