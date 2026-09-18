#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "HubConfig.h"

class HubStorage {
public:
  void begin() { prefs.begin("waterhub", false); }

  void loadConfig(HubConfig &cfg) {
    cfg.hubId = prefs.getString("hub", cfg.hubId);
    cfg.networkId = prefs.getString("net", cfg.networkId);
    cfg.wifiSsid = prefs.getString("ssid", cfg.wifiSsid);
    cfg.wifiPassword = prefs.getString("pass", cfg.wifiPassword);
    cfg.googleScriptUrl = prefs.getString("url", cfg.googleScriptUrl);
    cfg.pollIntervalSec = prefs.getUInt("poll", cfg.pollIntervalSec);
    cfg.txPowerDbm = prefs.getChar("pwr", cfg.txPowerDbm);
    cfg.configVersion = prefs.getUShort("ver", cfg.configVersion);

    for (uint8_t i = 0; i < cfg.nodeCount; ++i) {
      cfg.nodes[i].nodeId = i + 1;
      cfg.nodes[i].enabled = true;
    }
  }

  void saveConfig(const HubConfig &cfg) {
    prefs.putString("hub", cfg.hubId);
    prefs.putString("net", cfg.networkId);
    prefs.putString("ssid", cfg.wifiSsid);
    prefs.putString("pass", cfg.wifiPassword);
    prefs.putString("url", cfg.googleScriptUrl);
    prefs.putUInt("poll", cfg.pollIntervalSec);
    prefs.putChar("pwr", cfg.txPowerDbm);
    prefs.putUShort("ver", cfg.configVersion);
  }

private:
  Preferences prefs;
};
