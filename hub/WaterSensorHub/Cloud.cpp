#include "Cloud.h"

void Cloud::begin(const HubConfig &cfg) {
  Serial.println("Cloud subsystem initialized.");
}

void Cloud::connectWiFi() {
  // Placeholder for provisioning-managed Wi-Fi credentials.
}

void Cloud::sendTelemetry(const Telemetry &t, const HubConfig &cfg) {
  if (cfg.googleScriptUrl.length() == 0) {
    Serial.println("No Google Apps Script URL configured; telemetry retained on serial only.");
    return;
  }
  connectWiFi();
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  if (!http.begin(cfg.googleScriptUrl)) return;
  http.addHeader("Content-Type", "application/json");

  String body = "{";
  body += "\"hub\":\"" + cfg.hubId + "\",";
  body += "\"network\":\"" + cfg.networkId + "\",";
  body += "\"node\":" + String(t.nodeId) + ",";
  body += "\"seq\":" + String(t.sequence) + ",";
  body += "\"temperature_mV\":" + String(t.tempMv) + ",";
  body += "\"do_mV\":" + String(t.doMv) + ",";
  body += "\"water_flow\":" + String(t.waterFlow) + ",";
  body += "\"air_flow\":" + String(t.airFlow) + ",";
  body += "\"battery_mV\":" + String(t.batteryMv) + ",";
  body += "\"rssi\":" + String(t.rssi) + ",";
  body += "\"snr\":" + String(t.snr) + ",";
  body += "\"config_version\":" + String(t.configVersion);
  body += "}";

  int code = http.POST(body);
  Serial.printf("Apps Script HTTP response: %d\n", code);
  http.end();
}

void Cloud::refreshConfig(HubConfig &cfg, HubStorage &storage) {
  // Reserved for the remote CONFIG-sheet pull.
}
