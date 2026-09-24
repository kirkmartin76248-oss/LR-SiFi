# Production Node Firmware Design

Hardware: ESP32-C3 SuperMini.

## Wake state machine

SLEEP -> WAKE -> SENSOR_POWER_ON -> SENSOR_DELAY -> SAMPLE -> BATTERY -> ESP_NOW -> TELEMETRY_ACK -> UPDATE_QUERY -> CONFIGURE_IF_NEEDED -> SENSOR_POWER_OFF -> SLEEP

The Node never connects to Wi-Fi and never waits for Google Apps Script/backend.

## Configuration fingerprint

The Node maintains an automatic 4-byte **Config Fingerprint** for its active device-executable configuration. The fingerprint is included in every telemetry packet.

The immediate Hub telemetry ACK includes:
- the relevant Config Fingerprint
- a `config_changed` flag

If `config_changed = 1`, the Node performs the normal UPDATE_QUERY transaction and accepts the complete configuration update only after validation. The new configuration and fingerprint are saved persistently before the Node uses them on a later cycle.

The fingerprint is automatic and is not customer-managed. `config_revision` remains the monotonic configuration delivery guard.

## Wake behavior

1. Wake from timer.
2. Turn the activity LED on.
3. Power sensors through the TPS22929 load switch.
4. Wait the single Node-wide Sensor Delay.
5. Sample all enabled sensor ports.
6. Read battery voltage through the locked 1 MΩ / 1 MΩ divider with 100 nF ADC filter.
7. Transmit telemetry to the assigned Hub.
8. Wait for immediate telemetry ACK and retry if necessary.
9. Send UPDATE_QUERY.
10. If NO_UPDATE, continue to shutdown.
11. If CONFIG_UPDATE, validate revision and Hub identity, persist it, then send CONFIG_ACK.
12. Power sensors off.
13. Turn activity LED off.
14. Enter deep sleep for the configured reporting interval.

## Sensor contract

ANALOG:
- Configure ADC input.
- No digital pull-up/down.
- Report raw millivolts.

DIGITAL:
- Active HIGH -> internal pull-down.
- Active LOW -> internal pull-up.
- Normalize to 1=active and 0=inactive.

The Node does not perform customer-specific temperature or dissolved-oxygen conversion.

## Battery

The always-connected divider is:
- 1.0 MΩ from battery to ADC
- 1.0 MΩ from ADC to ground
- 100 nF ADC filter capacitor

Firmware computes battery voltage from the measured divider voltage using a 2:1 ratio and reports battery millivolts.

## Universal identity

Factory/default identity: NODE XXX.

The Installer writes the customer Node ID, Hub ID, actual Hub MAC, port configuration, reporting interval, Sensor Delay, and configuration revision to persistent memory.

All customer Nodes use the same firmware image.

## BLE

BLE is used only for local provisioning/configuration.

Installer workflow:
Configure -> Write -> Read Back/Verify -> Register -> Test

The Node exposes its current configuration and factory identity for the Installer to read.

## Retry behavior

Telemetry retries use the same sequence number.

If no telemetry ACK is received, the Node retries a bounded number of times. After exhausting retries, it records the communication failure locally and proceeds to sleep rather than waiting indefinitely.

A missed configuration update is safe because the backend can continue to stage the update in the Hub pending mailbox.
