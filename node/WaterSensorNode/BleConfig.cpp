#include "BleConfig.h"
#include "Config.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

static const char *SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0";
static const char *CONFIG_UUID  = "12345678-1234-5678-1234-56789abcdef1";
static const char *COMMAND_UUID = "12345678-1234-5678-1234-56789abcdef2";

NodeBleConfig *NodeBleConfig::instance_ = nullptr;

static bool jsonLong(const String &json, const char *key, long &out) {
  String needle = String("\"") + key + "\"";
  int p = json.indexOf(needle);
  if (p < 0) return false;
  p = json.indexOf(':', p + needle.length());
  if (p < 0) return false;
  int e = p + 1;
  while (e < (int)json.length() && isspace((unsigned char)json[e])) ++e;
  int end = e;
  while (end < (int)json.length() && json[end] != ',' && json[end] != '}') ++end;
  String v = json.substring(e, end);
  v.trim();
  if (!v.length()) return false;
  out = v.toInt();
  return true;
}

class NodeBleConfigCallbacks : public BLECharacteristicCallbacks {
public:
  void onWrite(BLECharacteristic *c) override {
    String v = c->getValue();
    if (NodeBleConfig::instance_) NodeBleConfig::instance_->pending_ = v;
  }
};

class NodeBleCommandCallbacks : public BLECharacteristicCallbacks {
public:
  void onWrite(BLECharacteristic *c) override {
    String v = c->getValue();
    if (NodeBleConfig::instance_) NodeBleConfig::instance_->handleCommand(v);
  }
};

class NodeBleServerCallbacks : public BLEServerCallbacks {
public:
  void onDisconnect(BLEServer *server) override {
    delay(100);
    server->startAdvertising();
  }
};

bool NodeBleConfig::begin(NodeConfig &cfg) {
  if (active_) return true;

  cfg_ = &cfg;
  instance_ = this;
  startedMs_ = millis();
  pending_.clear();

  String name = "WATER-NODE-" + String(cfg.nodeId);
  BLEDevice::init(name.c_str());
  BLEDevice::setMTU(517);

  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new NodeBleServerCallbacks());

  BLEService *service = server->createService(SERVICE_UUID);

  BLECharacteristic *configChar = service->createCharacteristic(
      CONFIG_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  configChar->setCallbacks(new NodeBleConfigCallbacks());
  configChar->setValue(toJson(cfg).c_str());

  BLECharacteristic *commandChar = service->createCharacteristic(
      COMMAND_UUID, BLECharacteristic::PROPERTY_WRITE);
  commandChar->setCallbacks(new NodeBleCommandCallbacks());

  service->start();

  BLEAdvertising *adv = server->getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  adv->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  active_ = true;
  Serial.println("BLE commissioning active.");
  Serial.println("Open the GitHub Pages Node Config page in Bluefy.");
  return true;
}

void NodeBleConfig::loop(NodeConfig &cfg) {
  if (!active_) return;

  if (pending_.length()) {
    String json = pending_;
    pending_.clear();

    if (json.startsWith("{")) {
      if (applyJson(cfg, json)) {
        saveConfig(cfg);
        Serial.println("BLE node configuration saved.");
      } else {
        Serial.println("BLE node configuration rejected.");
      }
    }
  }

  if (timedOut()) {
    Serial.println("BLE commissioning timeout.");
    stop();
  }
}

bool NodeBleConfig::timedOut() const {
  return active_ && (millis() - startedMs_ >= 300000UL);
}

void NodeBleConfig::stop() {
  if (!active_) return;
  BLEDevice::stopAdvertising();
  BLEDevice::deinit(true);
  active_ = false;
  instance_ = nullptr;
}

String NodeBleConfig::toJson(const NodeConfig &c) {
  String s = "{";
  s += "\"node\":" + String(c.nodeId);
  s += ",\"hub\":" + String(c.hubId);
  s += ",\"network\":" + String(c.networkId);
  s += ",\"interval\":" + String(c.reportIntervalSec);
  s += ",\"freq\":" + String(c.frequencyHz);
  s += ",\"sf\":" + String(c.spreadingFactor);
  s += ",\"bw\":" + String(c.bandwidthHz);
  s += ",\"cr\":" + String(c.codingRate);
  s += ",\"power\":" + String(c.txPowerDbm);
  s += ",\"version\":" + String(c.configVersion);
  s += "}";
  return s;
}

bool NodeBleConfig::applyJson(NodeConfig &c, const String &json) {
  long v;

  if (jsonLong(json, "node", v)) {
    if (v < 1 || v > 255) return false;
    c.nodeId = (uint8_t)v;
  }
  if (jsonLong(json, "hub", v)) {
    if (v < 1 || v > 255) return false;
    c.hubId = (uint8_t)v;
  }
  if (jsonLong(json, "network", v)) {
    if (v < 0 || v > 255) return false;
    c.networkId = (uint8_t)v;
  }
  if (jsonLong(json, "interval", v)) {
    if (v < MIN_INTERVAL_SEC || v > MAX_INTERVAL_SEC) return false;
    c.reportIntervalSec = (uint32_t)v;
  }
  if (jsonLong(json, "freq", v)) {
    if (v < 800000000L || v > 1000000000L) return false;
    c.frequencyHz = (uint32_t)v;
  }
  if (jsonLong(json, "sf", v)) {
    if (v < 5 || v > 12) return false;
    c.spreadingFactor = (uint8_t)v;
  }
  if (jsonLong(json, "bw", v)) {
    if (v != 62500 && v != 125000 && v != 250000 && v != 500000) return false;
    c.bandwidthHz = (uint32_t)v;
  }
  if (jsonLong(json, "cr", v)) {
    if (v < 5 || v > 8) return false;
    c.codingRate = (uint8_t)v;
  }
  if (jsonLong(json, "power", v)) {
    if (v < -9 || v > 22) return false;
    c.txPowerDbm = (int8_t)v;
  }
  if (jsonLong(json, "version", v)) {
    if (v < 0 || v > 65535) return false;
    c.configVersion = (uint16_t)v;
  }

  return true;
}

void NodeBleConfig::handleCommand(const String &cmd) {
  if (cmd == "REBOOT") {
    delay(150);
    ESP.restart();
  }
}
