#include "NodeConfig.h"
#include "Config.h"
#include <Preferences.h>

static Preferences prefs;

static void defaults(NodeConfig& c) {
  c.networkId = DEFAULT_NETWORK_ID;
  c.hubId = DEFAULT_HUB_ID;
  c.nodeId = DEFAULT_NODE_ID;
  c.reportIntervalSec = DEFAULT_INTERVAL_SEC;
  c.frequencyHz = DEFAULT_FREQUENCY_HZ;
  c.spreadingFactor = DEFAULT_SF;
  c.bandwidthHz = DEFAULT_BW_HZ;
  c.codingRate = DEFAULT_CR;
  c.txPowerDbm = DEFAULT_TX_POWER_DBM;
  c.configVersion = 1;
}

bool loadConfig(NodeConfig& c) {
  defaults(c);
  if (!prefs.begin("wsnode", true)) return false;

  c.networkId = prefs.getUChar("net", c.networkId);
  c.hubId = prefs.getUChar("hub", c.hubId);
  c.nodeId = prefs.getUChar("node", c.nodeId);
  c.reportIntervalSec = prefs.getULong("int", c.reportIntervalSec);
  c.frequencyHz = prefs.getULong("freq", c.frequencyHz);
  c.spreadingFactor = prefs.getUChar("sf", c.spreadingFactor);
  c.bandwidthHz = prefs.getULong("bw", c.bandwidthHz);
  c.codingRate = prefs.getUChar("cr", c.codingRate);
  c.txPowerDbm = prefs.getChar("pwr", c.txPowerDbm);
  c.configVersion = prefs.getUShort("ver", c.configVersion);

  prefs.end();

  if (c.reportIntervalSec < MIN_INTERVAL_SEC) c.reportIntervalSec = MIN_INTERVAL_SEC;
  if (c.reportIntervalSec > MAX_INTERVAL_SEC) c.reportIntervalSec = MAX_INTERVAL_SEC;
  return true;
}

bool saveConfig(const NodeConfig& c) {
  if (!prefs.begin("wsnode", false)) return false;
  prefs.putUChar("net", c.networkId);
  prefs.putUChar("hub", c.hubId);
  prefs.putUChar("node", c.nodeId);
  prefs.putULong("int", c.reportIntervalSec);
  prefs.putULong("freq", c.frequencyHz);
  prefs.putUChar("sf", c.spreadingFactor);
  prefs.putULong("bw", c.bandwidthHz);
  prefs.putUChar("cr", c.codingRate);
  prefs.putChar("pwr", c.txPowerDbm);
  prefs.putUShort("ver", c.configVersion);
  prefs.end();
  return true;
}
