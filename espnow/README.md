# ESP-NOW Reference / Legacy Work

This directory contains earlier ESP-NOW relay experiments and bench firmware. It is retained for reference.

The current production specification is in:
- docs/WIFI_WILLIAM_SYSTEM_ARCHITECTURE.md
- protocol/ESP_NOW_PROTOCOL.md
- protocol/device_config_schema.md
- protocol/espnow_protocol.h
- firmware/hub/PRODUCTION_DESIGN.md
- backend/ESP_NOW_BACKEND_CONTRACT.md

## Current production hardware

- Node: ESP32-C3 SuperMini
- Hub: Seeed XIAO ESP32-C6
- Hub antenna: U.FL/external 2.4 GHz antenna
- Runtime: Node -> ESP-NOW -> Hub -> Wi-Fi -> customer backend

Older files in this directory may refer to different node hardware, relay behavior, polling, or configuration assumptions. Do not treat those files as the production source of truth.
