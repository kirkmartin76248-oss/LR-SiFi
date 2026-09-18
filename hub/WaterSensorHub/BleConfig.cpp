#include "BleConfig.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

static const char *SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0";
static const char *CONFIG_UUID  = "12345678-1234-5678-1234-56789abcdef1";
static const char *COMMAND_UUID = "12345678-1234-5678-1234-56789abcdef2";

HubBleConfig *HubBleConfig::instance_ = nullptr;

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

static bool jsonString(const String &json, const char *key, String &out) {
  String needle = String("\"") + key + "\"";
  int p = json.indexOf(needle);
  if (p < 0) return false;
  p = json.indexOf(':', p + needle.length());
  if (p < 0) return false;
  int e = p + 1;
  while (e < (int)json.length() && isspace((unsigned char)json[e])) ++e;
  if (e >= (int)json.length() || json[e] != '\"') return false;
  ++e;
  int end = json.indexOf('\"', e);
  if (end < 0) return false;
  out = json.substring(e, end);
  return true;
}

class HubBleConfigCallbacks : public BLECharacteristicCallbacks {
public:
  void onWrite(BLECharacteristic *c) override {
    String v = c->getValue();
    if (HubBleConfig::instance_) HubBleConfig::instance_->pending_ = v;
  }
};

class HubBleCommandCallbacks : public BLECharacteristicCallbacks {
public:
  void onWrite(BLECharacteristic *c) override {
    String v = c->getValue();
    if (HubBleConfig::instance_) HubBleConfig::instance_->handleCommand(v);
  }
};

class HubBleServerCallbacks : public BLEServerCallbacks {
public:
  void onDisconnect(BLEServer *server) override {
    delay(100);
    server->startAdvertising();
  }
};

bool HubBleConfig::begin(HubConfig &cfg, HubStorage &storage) {
  if (active_) return true;

  cfg_ = &cfg;
  storage_ = &storage;
  instance_ = this;
  startedMs_ = millis();
  pending_.clear();

  String name = "WATER-HUB-" + cfg.hubId;
  BLEDevice::init(name.c_str());
  BLEDevice::setMTU(517);

  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new HubBleServerCallbacks());

  BLEService *service = server->createService(SERVICE_UUID);

  BLECharacteristic *configChar = service->createCharacteristic(
      CONFIG_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  configChar->setCallbacks(new HubBleConfigCallbacks());
  configChar->setValue(toJson(cfg).c_str());

  BLECharacteristic *commandChar = service->createCharacteristic(
      COMMAND_UUID, BLECharacteristic::PROPERTY_WRITE);
  commandChar->setCallbacks(new HubBleCommandCallbacks());

  service->start();

  BLEAdvertising *adv = server->getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  adv->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  active_ = true;
  Serial.println("BLE commissioning active.");
  Serial.println("Open the GitHub Pages Hub Config page in Bluefy.");
  return true;
}

void HubBleConfig::loop(HubConfig &cfg, HubStorage &storage) {
  if (!active_) return;

  if (pending_.length()) {
    String json = pending_;
    pending_.clear();

    if (json.startsWith("{")) {
      if (applyJson(cfg, json)) {
        storage.saveConfig(cfg);
        Serial.println("BLE hub configuration saved.");
      } else {
        Serial.println("BLE hub configuration rejected.");
      }
    }
  }

  if (timedOut()) {
    Serial.println("BLE commissioning timeout.");
    stop();
  }
}

bool HubBleConfig::timedOut() const {
  return active_ && (millis() - startedMs_ >= 300000UL);
}

void HubBleConfig::stop() {
  if (!active_) return;
  BLEDevice::stopAdvertising();
  BLEDevice::deinit(true);
  active_ = false;
  instance_ = nullptr;
}

String HubBleConfig::toJson(const HubConfig &c) {
  String s = "{";
  s += "\"hub\":\"" + c.hubId + "\"";
  s += ",\"network\":\"" + c.networkId + "\"";
  s += ",\"ssid\":\"" + c.wifiSsid + "\"";
  s += ",\"password\":\"" + c.wifiPassword + "\"";
  s += ",\"url\":\"" + c.googleScriptUrl + "\"";
  s += ",\"poll\":" + String(c.pollIntervalSec);
  s += ",\"freq\":" + String((uint32_t)(c.frequencyMHz * 1000000.0f));
  s += ",\"sf\":" + String(c.spreadingFactor);
  s += ",\"bw\":" + String((uint32_t)(c.bandwidthKHz * 1000.0f));
  s += ",\"cr\":" + String(c.codingRate);
  s += ",\"power\":" + String(c.txPowerDbm);
  s += ",\"preamble\":" + String(c.preambleSymbols);
  s += ",\"nodes\":" + String(c.nodeCount);
  s += ",\"version\":" + String(c.configVersion);
  s += "}";
  return s;
}

bool HubBleConfig::applyJson(HubConfig &c, const String &json) {
  long v;
  String s;

  if (jsonString(json, "hub", s)) {
    if (!s.length() || s.length() > 31) return false;
    c.hubId = s;
  }
  if (jsonString(json, "network", s)) {
    if (!s.length() || s.length() > 31) return false;
    c.networkId = s;
  }
  if (jsonString(json, "ssid", s)) {
    if (s.length() > 63) return false;
    c.wifiSsid = s;
  }
  if (jsonString(json, "password", s)) {
    if (s.length() > 63) return false;
    c.wifiPassword = s;
  }
  if (jsonString(json, "url", s)) {
    if (s.length() > 255) return false;
    c.googleScriptUrl = s;
  }
  if (jsonLong(json, "poll", v)) {
    if (v < 5 || v > 43200) return false;
    c.pollIntervalSec = (uint32_t)v;
  }
  if (jsonLong(json, "freq", v)) {
    if (v < 800000000L || v > 1000000000L) return false;
    c.frequencyMHz = (float)v / 1000000.0f;
  }
  if (jsonLong(json, "sf", v)) {
    if (v < 5 || v > 12) return false;
    c.spreadingFactor = (uint8_t)v;
  }
  if (jsonLong(json, "bw", v)) {
    if (v != 62500 && v != 125000 && v != 250000 && v != 500000) return false;
    c.bandwidthKHz = (float)v / 1000.0f;
  }
  if (jsonLong(json, "cr", v)) {
    if (v < 5 || v > 8) return false;
    c.codingRate = (uint8_t)v;
  }
  if (jsonLong(json, "power", v)) {
    if (v < -9 || v > 22) return false;
    c.txPowerDbm = (int8_t)v;
  }
  if (jsonLong(json, "preamble", v)) {
    if (v < 8 || v > 65535) return false;
    c.preambleSymbols = (uint16_t)v;
  }
  if (jsonLong(json, "nodes", v)) {
    if (v < 1 || v > MAX_NODES) return false;
    c.nodeCount = (uint8_t)v;
    for (uint8_t i = 0; i < c.nodeCount; ++i) {
      c.nodes[i].nodeId = i + 1;
      c.nodes[i].enabled = true;
    }
  }
  if (jsonLong(json, "version", v)) {
    if (v < 0 || v > 65535) return false;
    c.configVersion = (uint16_t)v;
  }

  return true;
}

void HubBleConfig::handleCommand(const String &cmd) {
  if (cmd == "REBOOT") {
    delay(150);
    ESP.restart();
  }
}
