# Hub Firmware

Production Hub firmware for the Seeed XIAO ESP32-C6.

Responsibilities:
- BLE provisioning
- persistent Hub identity/configuration
- ESP-NOW receive
- Wi-Fi connection
- Apps Script/backend forwarding
- Hub status reporting

The Hub has no sensors.

## XIAO ESP32-C6 RF-switch control

The XIAO ESP32-C6 has an onboard FM8625H RF switch between the ESP32-C6 RF path and the two antenna paths. The production Hub uses the **U.FL/external antenna**.

From the XIAO ESP32-C6 V1.0 schematic:

- RF-switch port-select control: **GPIO14**
  - LOW (0) = RF1
  - HIGH (1) = RF2
- RF-switch power control: **GPIO3**
  - LOW (0) = RF switch ON
  - HIGH (1) = RF switch OFF
- The U.FL/external antenna path is **RF2**.

Therefore Hub startup must explicitly configure the RF switch before initializing Wi-Fi/ESP-NOW:

1. Configure GPIO3 and GPIO14 as outputs.
2. Select RF2: GPIO14 = HIGH.
3. Enable the RF switch: GPIO3 = LOW.
4. Initialize the ESP32-C6 wireless stack.
5. Verify wireless operation using the external/U.FL antenna path.

Do not assume the antenna switch defaults are correct after reset; firmware owns these GPIO states.

## Hub / Node communication

The Hub does not maintain a Node list or Node configuration database.

Node wake transaction:

1. Node sends telemetry.
2. Hub immediately ACKs the telemetry.
3. Node asks whether the Hub has a pending configuration update.
4. Hub checks internal pending-update memory keyed by Node ID.
5. Hub returns NO_UPDATE, or sends the pending update.
6. Node stores/acknowledges the update and sleeps.
7. Separately, Hub forwards the telemetry to the customer backend with Hub timestamp and Hub-measured RSSI.
8. Backend remains the authoritative configuration source and can send a new update to the Hub for pending delivery to the Node.

The Node wake cycle must never wait for Google Apps Script/backend connectivity.
