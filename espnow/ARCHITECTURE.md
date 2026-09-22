# Krek Labs ESP-NOW Architecture — Locked

## 1. Network
The relay is always listening. Nodes wake on their own schedules, measure sensors, transmit by ESP-NOW unicast, receive an application ACK, check for configuration, then sleep.

The relay independently maintains Wi-Fi/Internet connectivity and forwards measurements to one Google Apps Script endpoint.

## 2. Node
DFRobot Beetle ESP32-C6 Mini DFR1117.

Locked sensor mapping:
- Temperature: GPIO4 / ADC
- Dissolved oxygen: GPIO5 / ADC
- Water flow: GPIO7 digital
- Air flow: GPIO21 digital
- TPS22929D sensor power enable: GPIO23
- Battery monitor: onboard GPIO0 divider
- Sensor power: 3V3 through TPS22929D
- Common sensor ground: GND

Node sequence:
Wake → sensor power ON → 2 s stabilization → read sensors → sensor power OFF → ESP-NOW measurement → ACK → CONFIG_CHECK → configuration exchange if required → sleep.

Analog payloads are raw mV. Digital payloads are 0/1. Google performs conversion/interpretation.

## 3. Relay
Seeed XIAO ESP32-C6:
- 5 V external supply → VBUS
- external supply ground → GND
- backup battery + → BAT
- backup battery − → GND
- external 2.4 GHz antenna → U.FL/ANT2

Relay jobs:
1. ESP-NOW sensor network
2. Wi-Fi/HTTPS/NTP backend

Wi-Fi channel is master for ESP-NOW. ESP-NOW uses the same channel.

The relay does not maintain a customer node database. Google is the configuration master. The relay only retains pending operational updates keyed by exact Node MAC.

## 4. Data
RAW_DATA is the source-of-truth raw history. Site DATA tabs contain converted/interpreted values.

Google structure:
- RELAY_CONFIG
- NODE_CONFIG
- ALERT_STATE
- RAW_DATA
- one DATA tab per site

Unknown valid nodes are stored in RAW_DATA but do not route to site DATA or trigger sensor alarms until registered.

## 5. Reliability
- Application ACK is immediate and local.
- Measurement duplicate identity: Node MAC + Sequence.
- Cloud transaction duplicate identity: requestId.
- Relay buffers measurements in internal flash during Internet outage.
- Backlog survives relay reboot.
- Oldest buffered records upload first.
- Records are only reclaimed after confirmed cloud acceptance.
- Pending node configuration survives relay reboot.
- No silent overwrite of undelivered measurements.

## 6. Alerts
Each site has two Discord webhooks:
- Alerts
- Monthly Status

Alerts are state based: one notification entering out-of-tolerance, no spam while persistent, one recovery notification, and a new alert if the condition returns later.

Monthly report covers communications, battery, temperature, DO, water flow, air flow, events, and device health.

## 7. Commissioning
Relay BLE:
Relay MAC read-only, Relay Name, Wi-Fi Network, Wi-Fi Password, Google Script URL, Time Zone, TEST CONNECTION, CONFIGURE.

Node BLE:
Node MAC read-only, Node Name, Relay MAC selection/discovery, Sleep Interval, Sensor Delay, RF Power, Sensor Types 1–4, CONFIGURE.

Node BLE window is 30 seconds; timer stops once connected.

Node stores relay MAC and operational settings. Site Location is not stored on the node.

## 8. Security choice
ESP-NOW is unicast with MAC-address routing and no PMK/LMK provisioning. This was explicitly selected for simplicity.

## 9. Current customer/business context
Krek Labs, LLC. Initial application is oyster restoration monitoring. Initial deployment is 5 nodes and 1 relay, with potential expansion later.
