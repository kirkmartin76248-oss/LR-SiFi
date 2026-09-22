# Configuration Model — Locked

## NODE_CONFIG
- Node MAC
- Node Name
- Site Location
- Relay MAC
- Sleep Interval
- Sensor Delay
- RF Power
- Sensor Type 1
- Sensor 1 Min
- Sensor 1 Max
- Sensor Type 2
- Sensor 2 Min
- Sensor 2 Max
- Sensor Type 3
- Sensor 3 Min
- Sensor 3 Max
- Sensor Type 4
- Sensor 4 Min
- Sensor 4 Max
- Config Fingerprint
- Last Contact
- Firmware Version
- Status

Sensor types:
NONE, TEMP, DO, WATER, AIR.

## RELAY_CONFIG
- Relay MAC
- Relay Name
- Wi-Fi SSID
- Wi-Fi Password
- Google Script URL
- Time Zone
- Config Fingerprint
- Last Contact
- Firmware Version
- Status

The relay only needs the Google Script URL. Google maps node MAC to Node Name, Site Location, sensor types and limits.

## DATA
Site DATA tabs contain converted/interpreted values only.

## RAW_DATA
RAW_DATA retains raw source history:
Timestamp, Relay MAC, Node MAC, Sequence, Sensor 1 Raw mV, Sensor 2 Raw mV, Sensor 3 Raw, Sensor 4 Raw, Battery mV, RSSI, Config Fingerprint.

## Processing order
1. Validate payload
2. Write RAW_DATA
3. Find Node MAC
4. Immediately determine pending config update
5. Convert/interpret values
6. Evaluate limits
7. Update ALERT_STATE
8. Send Discord state-change alert/recovery
9. Write converted site DATA
