# Node Firmware

Production Node firmware for the ESP32-C3 SuperMini.

Responsibilities:
- BLE provisioning
- persistent Node configuration
- sensor power control
- Node-wide Sensor Delay
- sensor acquisition
- battery measurement
- ESP-NOW telemetry
- sleep/wake operation

The firmware must remain universal across customer deployments. Customer-specific alert limits and analog conversion/calibration remain server-side unless a future hardware requirement explicitly requires local storage.
