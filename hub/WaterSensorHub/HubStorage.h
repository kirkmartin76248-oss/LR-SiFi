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
    cfg.googleScriptUrl = prefs.getString("url", cfg.googleScriptUrl);
    cfg.pollIntervalSec = prefs.getUInt("poll", cfg.pollIntervalSec);
    cfg.txPowerDbm = prefs.getChar("pwr", cfg.txPowerDbm);
    for (uint8_t i = 0; i < cfg.nodeCount; ++i) {
      cfg.nodes[i].nodeId = i + 1;
      cfg.nodes[i].enabled = true;
    }
  }

  void saveConfig(const HubConfig &cfg) {
    prefs.putString("hub", cfg.hubId);
    prefs.putString("net", cfg.networkId);
    prefs.putString("url", cfg.googleScriptUrl);
    prefs.putUInt("poll", cfg.pollIntervalSec);
    prefs.putChar("pwr", cfg.txPowerDbm);
  }

private:
  Preferences prefs;
};
