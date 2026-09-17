#include <Arduino.h>
#include "Config.h"
#include "NodeConfig.h"
#include "Sensors.h"
#include "LoRaRadio.h"
#include "Protocol.h"
#include "PowerManager.h"

NodeConfig cfg;
LoRaRadio radio;
uint32_t sequenceNo = 0;

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LED_OFF);

  Serial.begin(115200);
  delay(40);

  loadConfig(cfg);
  initSensors();
  initBatteryADC();

  // GPIO7 is the SX1262 DIO1 wake source. It must be LOW while asleep.
  pinMode(PIN_RADIO_DIO1, INPUT);

  bool radioWake = (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1);

  if (!radio.begin(cfg)) {
    // Leave LED on as a hard fault indication.
    digitalWrite(PIN_LED, LED_ON);
    delay(500);
    goToDeepSleep();
  }

  digitalWrite(PIN_LED, LED_ON);

  if (radioWake) {
    handleRadioWake();
  } else {
    // First boot / reset: initialize duty-cycle RX and sleep.
    radio.startDutyCycleRX();
  }

  digitalWrite(PIN_LED, LED_OFF);
  goToDeepSleep();
}

void loop() {
  // Never reached: this node is event-driven and uses deep sleep.
}

void handleRadioWake() {
  PacketHeader hdr{};
  PollPacket poll{};

  if (!radio.receivePoll(hdr, poll)) {
    radio.startDutyCycleRX();
    return;
  }

  if (hdr.networkId != cfg.networkId ||
      hdr.hubId != cfg.hubId ||
      hdr.nodeId != cfg.nodeId ||
      hdr.type != PKT_POLL) {
    radio.startDutyCycleRX();
    return;
  }

  // Hub has addressed this node.
  sensorsPower(true);
  delay(SENSOR_SETTLE_MS);

  TelemetryPacket t{};
  t.version = PROTOCOL_VERSION;
  t.type = PKT_TELEMETRY;
  t.networkId = cfg.networkId;
  t.hubId = cfg.hubId;
  t.nodeId = cfg.nodeId;
  t.sequence = ++sequenceNo;

  readTelemetry(t);

  sensorsPower(false);

  radio.sendTelemetry(t);

  AckPacket ack{};
  if (radio.receiveAck(ack, ACK_TIMEOUT_MS)) {
    if (ack.networkId == cfg.networkId &&
        ack.hubId == cfg.hubId &&
        ack.nodeId == cfg.nodeId &&
        ack.sequence == t.sequence) {

      if (ack.configVersion != cfg.configVersion) {
        cfg.reportIntervalSec = ack.reportIntervalSec;
        cfg.txPowerDbm = ack.txPowerDbm;
        cfg.configVersion = ack.configVersion;
        saveConfig(cfg);
        radio.applyConfig(cfg);
      }
    }
  }

  radio.startDutyCycleRX();
}
