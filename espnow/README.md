# Krek Labs ESP-NOW Water Monitoring Network

This folder contains the current locked ESP-NOW architecture and relay firmware work for the Krek Labs water-monitoring project.

## System
Sensor Node → ESP-NOW LR → XIAO ESP32-C6 Relay → Wi-Fi → Google Apps Script/Sheets → Discord

## Hardware
- Remote node: DFRobot Beetle ESP32-C6 Mini DFR1117
- Relay: Seeed Studio XIAO ESP32-C6
- Relay antenna: external 2.4 GHz 7 dBi antenna through U.FL
- Relay power: external 5 V to VBUS
- Relay backup battery: 3.7 V Li-ion to BAT
- No external RTC
- No external storage
- Sensor node battery: 1S 3.7 V battery
- Sensor power switched by TPS22929D

## Current status
Architecture is locked. Relay firmware v0.1 has been generated for bench testing. BLE commissioning, production TLS validation, OTA, final flash wear optimization, and final watchdog policy remain before field release.

See:
- ARCHITECTURE.md
- PROTOCOL.md
- CONFIG.md
- BACKEND.md
- STATUS.md
- relay/relay.ino
