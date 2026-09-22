# Implementation Status

## Locked
- ESP-NOW architecture
- Node/relay roles
- DFR1117 node pin map
- XIAO ESP32-C6 relay
- relay VBUS/BAT power arrangement
- external U.FL antenna
- ESP-NOW LR
- unicast MAC routing
- packet protocol
- application ACK
- configuration exchange
- Google JSON protocol
- persistent outage buffering concept
- raw-vs-converted data ownership
- Discord alert/report architecture
- BLE commissioning fields

## Firmware v0.1 generated
- Wi-Fi
- NTP
- ESP-NOW initialization
- LR protocol configuration
- packet parsing
- ACKs
- duplicate detection
- LittleFS spool
- pending configuration persistence
- HTTPS cloud upload framework
- requestId
- diagnostics

## Remaining before field release
- BLE commissioning implementation
- Production TLS certificate validation
- Final flash wear-optimized spool/compaction
- Full cloud accepted-list validation
- OTA
- final watchdog/recovery behavior
- end-to-end node firmware
- hardware bench testing
- Google Apps Script implementation/testing
