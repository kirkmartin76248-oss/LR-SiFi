/*
  KREK LABS - XIAO ESP32-C6 SIMPLE ESP-NOW RELAY RECEIVER

  Purpose:
    Bench-test receiver for the ESP32-C3 Krek node.

  Current radio mode:
    Standard ESP-NOW only.
    LR compatibility between C3 and C6 will be tested separately.

  IMPORTANT:
    ESP-NOW channel must match the C3 node.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

constexpr uint8_t ESP_NOW_CHANNEL = 1;

struct KrekPacket {
  uint32_t sequence;
  uint16_t sensor1;
  uint16_t sensor2;
  uint16_t sensor3;
  uint16_t sensor4;
  uint16_t batteryADC;
};

void onDataRecv(
  const esp_now_recv_info_t *info,
  const uint8_t *data,
  int len
)
{
  Serial.println();
  Serial.println("========== PACKET RECEIVED ==========");

  Serial.print("Length: ");
  Serial.println(len);

  Serial.print("From MAC: ");

  for (int i = 0; i < 6; i++) {
    if (info->src_addr[i] < 16) Serial.print("0");
    Serial.print(info->src_addr[i], HEX);
    if (i < 5) Serial.print(":");
  }

  Serial.println();

  if (len != sizeof(KrekPacket)) {
    Serial.println("Unexpected packet size.");
    Serial.println("====================================");
    return;
  }

  KrekPacket packet;
  memcpy(&packet, data, sizeof(packet));

  Serial.print("Sequence: ");
  Serial.println(packet.sequence);

  Serial.print("Sensor 1 ADC: ");
  Serial.println(packet.sensor1);

  Serial.print("Sensor 2 ADC: ");
  Serial.println(packet.sensor2);

  Serial.print("Sensor 3 ADC: ");
  Serial.println(packet.sensor3);

  Serial.print("Sensor 4 ADC: ");
  Serial.println(packet.sensor4);

  Serial.print("Battery ADC: ");
  Serial.println(packet.batteryADC);

  Serial.println("====================================");
}

void setup()
{
  Serial.begin(115200);

  delay(500);

  Serial.println();
  Serial.println("================================");
  Serial.println("KREK C6 ESP-NOW RELAY RECEIVER");
  Serial.println("================================");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.print("Relay MAC: ");
  Serial.println(WiFi.macAddress());

  esp_err_t result = esp_wifi_set_channel(
    ESP_NOW_CHANNEL,
    WIFI_SECOND_CHAN_NONE
  );

  if (result != ESP_OK) {
    Serial.print("ERROR: channel setup failed: ");
    Serial.println(result);
    return;
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW initialization failed");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.print("Listening on ESP-NOW channel ");
  Serial.println(ESP_NOW_CHANNEL);

  Serial.println("Waiting for Krek C3 packets...");
}

void loop()
{
  // Receiver remains awake continuously.
}
