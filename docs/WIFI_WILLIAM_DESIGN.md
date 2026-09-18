# WiFi William — Locked Design Specification

**Project:** EcoSYNC Technologies — WiFi William  
**Controller:** Seeed Studio XIAO ESP32-C6  
**Primary application:** Battery-powered water/oyster-tank monitoring  
**Initial customer/application:** Chesapeake Bay Foundation (CBF) oyster restoration tanks

> This document records the current locked architecture and design decisions.

---

## 1. Product Architecture

WiFi William is a battery-powered wireless sensor node built around the XIAO ESP32-C6.

- No custom PCB.
- Electronics are mounted to a removable 3D-printed tray that slides into the enclosure tube.
- Enclosure target is approximately a 2-inch round tube with screw-on caps.
- External sensors use waterproof field-serviceable connectors.
- External antenna uses the XIAO U.FL path rather than the onboard ceramic antenna.
- Nominal measurement interval: 1 hour.
- Supported measurement interval: 5 minutes to 24 hours.
- Battery target: approximately 6 months / one oyster season at nominal operation.
- Target total sleep current: ideally <=20 uA.
- Main electronics enclosure cost target: approximately $30, excluding sensors.

## 2. Power Architecture — LOCKED

Battery: 1S 18650 lithium-ion, 3500 mAh nominal, approximately $4.

Power path:
Battery + -> physical ON/OFF switch -> XIAO BAT pin.

The XIAO regulated 3.3 V rail supplies the switched sensor power rail.

TPS22929 load switch:
- Input: XIAO 3.3 V
- Output: switched 3.3 V sensor rail
- Enable: GPIO21
- Recommended 100 kOhm EN pulldown to keep the sensor rail OFF during reset/boot

The switched sensor rail powers the dissolved oxygen sensor, temperature divider, water-flow paddle network, and air-flow paddle network.

There is no external ADC.

## 3. XIAO ESP32-C6 Pin Map — LOCKED

| XIAO pin | ESP32-C6 GPIO | Function |
|---|---:|---|
| D0 / A0 | GPIO0 / ADC1_CH0 | Battery monitor |
| D1 / A1 | GPIO1 / ADC1_CH1 | Temperature |
| D2 / A2 | GPIO2 / ADC1_CH2 | Dissolved oxygen |
| Bottom pad | GPIO4 / ADC1_CH4 | Water-flow paddle |
| Bottom pad | GPIO5 / ADC1_CH5 | Air-flow paddle |
| Bottom pad | GPIO6 / ADC1_CH6 | Spare analog |
| D3 | GPIO21 | Sensor power enable |
| D4 | GPIO22 | Spare |
| D5 | GPIO23 | Spare |
| D6 | GPIO16 | Spare |
| D7 | GPIO17 | Spare |
| D8 | GPIO19 | Spare |
| D9 | GPIO20 | Spare |
| D10 | GPIO18 | Spare |
| — | GPIO14 | RF antenna selection — RESERVED |
| — | GPIO15 | RF switch power + onboard LED — RESERVED |

GPIO14 and GPIO15 are reserved and must not be assigned to sensors or the load switch.

## 4. External Antenna / RF Switch — LOCKED

XIAO RF switch: FM8625H.

- RF1 = onboard ceramic antenna
- RF2 = U.FL antenna path
- GPIO14 = RF switch port selection
- GPIO15 = RF switch power

Required behavior:
- GPIO14 HIGH selects RF2 / U.FL.
- GPIO15 LOW powers the RF switch ON.
- During deep sleep, GPIO14 remains HIGH and GPIO15 goes HIGH so the U.FL path remains selected while the RF switch and onboard LED are OFF.

External RF path:
XIAO U.FL -> coax/pigtail -> enclosure bulkhead/feed-through -> external antenna.

The onboard ceramic antenna is not used.

## 5. Onboard LED — LOCKED

The XIAO onboard LED is the wake/activity indicator.

- LED ON while awake and working.
- LED OFF before deep sleep.
- Brief active LED current is acceptable.
- GPIO15 is shared with RF switch power, so the shared function must be handled deliberately.

Actual boot/reset behavior should be validated on hardware.

## 6. Analog Inputs

All four sensor inputs are treated as analog by firmware, including both paddle switches.

### Input 1 — Temperature

GPIO1 / ADC1_CH1.

Sensor: DROK 10 kOhm NTC thermistor probe, B3950, +/-1%, approximately -25 to 125 C.

Circuit:
SWITCHED 3.3V -> 10k 1% resistor -> GPIO1 node -> 10k NTC B3950 -> GND, with 100 nF from the ADC node to GND.

Firmware transmits raw ADC voltage in millivolts. Conversion is server-side.

### Input 2 — Dissolved Oxygen

GPIO2 / ADC1_CH2.

Sensor: DFRobot Gravity Analog Dissolved Oxygen SEN0237-A.
- Supply: 3.3–5.5 V
- Analog output: 0–3.0 V
- Galvanic BNC probe
- 100 nF filtering
- Power switched by the load switch

Connections:
SWITCHED 3.3V -> DO+
GND -> DO-
DO analog -> GPIO2 / A2, with 100 nF to GND.

### Input 3 — Water-flow paddle

GPIO4 / ADC1_CH4.

Analog network:
SWITCHED 3.3V -> 10k -> GPIO4 node.
GPIO4 node -> 10k -> paddle switch -> GND.
100 nF from GPIO4 node to GND.

Nominal states:
- Open: approximately 3.3 V
- Closed: approximately 1.65 V
- Hard short: approximately 0 V

### Input 4 — Air-flow paddle

GPIO5 / ADC1_CH5.

Identical analog network to the water-flow paddle.

### Spare analog

GPIO6 / ADC1_CH6 remains available as a spare analog input.

## 7. Battery Monitor — LOCKED

Always-connected 1.0 MOhm / 1.0 MOhm divider.

BATTERY+ -> 1.0 MOhm -> GPIO0/A0 node -> 1.0 MOhm -> GND.

100 nF from the ADC node to GND.

Because the divider is 1:1:
Battery voltage = ADC node voltage x 2.

Firmware transmits battery voltage in millivolts.

## 8. Sensor Field Connections — LOCKED

Each of the four sensors gets its own waterproof 3-pin connector pair.

Each connection carries:
1. Switched 3.3 V
2. Ground
3. Analog signal

Each enclosure cable penetration uses a PG7 waterproof cable gland.

Physical arrangement:
Sensor -> sensor cable -> waterproof 3-pin connector pair -> short pigtail -> PG7 cable gland -> electronics enclosure.

Customer service workflow:

Before season:
- Plug in the four sensor harnesses.

After season:
- Unplug the four sensor harnesses.
- Store sensors separately.
- Leave the electronics enclosure sealed.

The electronics enclosure does not need to be opened and the customer does not need to splice wires.

All four physical ports are identical. Sensor 1–4 assignments are determined by configuration/software rather than different connector types.

Current estimated cost:
- Four waterproof connector/gland assemblies
- Approximately $3 each
- Approximately $12 per device

This cost is intentionally retained because it provides simple seasonal field service.

## 9. Wake / Measurement / Sleep Sequence — LOCKED

1. Wake from deep sleep.
2. Turn onboard LED ON.
3. Turn Wi-Fi ON.
4. Allow approximately 500 ms for Wi-Fi startup/connection activity.
5. Turn sensor power ON through the load switch.
6. Wait for sensor stabilization; default 2 seconds.
7. Read all four analog inputs.
8. Read battery voltage.
9. POST measurements to the configured customer/application server.
10. Receive current runtime settings if available.
11. Save latest runtime settings to nonvolatile storage.
12. Turn sensor power OFF.
13. Turn Wi-Fi OFF.
14. Turn onboard LED OFF.
15. Enter deep sleep for the configured interval.

Wi-Fi timeout is hard-coded in firmware and is not a normal customer configuration field.

## 10. Runtime Configuration

The ESP32 retrieves only values that can change device behavior during normal operation:
- Sleep duration
- Sensor stabilization delay
- Monitoring Enabled

These runtime values are checked on every wake.

There is no configuration version field and no customer publish/update-version step.

If the server is unavailable, the device uses the last successfully cached runtime values.

## 11. Monitoring Enabled vs Alerts Enabled

### Monitoring Enabled

Read by the ESP32 every wake.

If TRUE:
- Power sensor rail.
- Wait for sensor stabilization.
- Measure all sensors.
- Transmit measurements.

If FALSE:
- Do not power sensors.
- Do not wait for sensor stabilization.
- Wake Wi-Fi only long enough to check/report status and obtain current runtime settings.
- Return to sleep.

Purpose: allow a tank/system to be suspended for weeks or months without physically touching the device while retaining remote re-enable capability.

A monitoring-disabled heartbeat may contain:
{
  "device_id": "CBF002",
  "status": "monitoring_disabled",
  "battery_mv": 4012
}

### Alerts Enabled

Apps Script only.

The ESP32 does not read, store, or receive Alerts Enabled.

Apps Script uses Alerts Enabled to decide whether to send alarm/recovery notifications while measurements continue to be recorded.

## 12. BLE Provisioning — LOCKED CONCEPT

BLE is used for initial setup and later reconfiguration.

When the device is powered on:
- BLE configuration window: 30 seconds

BLE configuration includes:
- Device ID
- Device Name
- Location
- Wi-Fi SSID
- Wi-Fi password
- Customer/application SERVER_URL
- Sensor type 1
- Sensor type 2
- Sensor type 3
- Sensor type 4
- Sleep duration
- Sensor stabilization delay

Wi-Fi credentials are stored only on the device and are never stored in the Google Sheet.

SERVER_URL is the customer/application endpoint. It is not a per-device authentication token.

If the endpoint URL changes, devices can be reconfigured over BLE without reflashing firmware.

## 13. Generic Firmware / Server Interface

Firmware remains sensor-agnostic as practical.

Normal measurement payload:
{
  "device_id": "CBF002",
  "battery_mv": 4012,
  "input1_mv": 1847,
  "input2_mv": 923,
  "input3_mv": 3270,
  "input4_mv": 0
}

The server converts raw values according to sensor types in CONFIG.

Expected runtime response:
{
  "sleep_seconds": 3600,
  "sensor_delay_seconds": 2,
  "monitoring_enabled": true
}

Alerts Enabled is not returned to the device.

## 14. Google / Cloud Architecture

Conceptual architecture:

Device -> Customer-specific Apps Script -> Central Google Sheet

Different customers/applications may have separate Apps Script endpoints while writing to the same central Google Sheet.

Example:
CBF devices -> CBF Apps Script -> central sheet

Another application:
Customer B devices -> Customer B Apps Script -> same central sheet

Each Apps Script stores its own sheet access information server-side.

The ESP32 stores only its configured SERVER_URL. It does not store central Google Sheet credentials.

## 15. CONFIG Sheet

Customer-facing fields:

| Field |
|---|
| Device ID |
| Device Name |
| Location |
| Sleep Duration |
| Sensor Delay |
| Monitoring Enabled |
| Alerts Enabled |
| Sensor 1 Type |
| Sensor 1 Min |
| Sensor 1 Max |
| Sensor 2 Type |
| Sensor 2 Min |
| Sensor 2 Max |
| Sensor 3 Type |
| Sensor 3 Min |
| Sensor 3 Max |
| Sensor 4 Type |
| Sensor 4 Min |
| Sensor 4 Max |

Sensor type fields should use dropdowns.

Do NOT put these in the customer-facing CONFIG sheet:
- Wi-Fi SSID
- Wi-Fi password
- SERVER_URL
- Per-device authentication token
- Configuration version

## 16. Automatic Device Registration

A newly provisioned device registers through its customer/application Apps Script.

Workflow:
1. Device powers on.
2. BLE is available for 30 seconds.
3. Phone provisions the device.
4. Device stores configuration.
5. Device connects to Wi-Fi.
6. Device sends provisioning/registration information to SERVER_URL.
7. Apps Script creates or updates the CONFIG row.
8. If the location is new, Apps Script creates the corresponding location data tab.
9. Wi-Fi credentials remain only on the device.
10. If the server is unavailable, the device retains BLE configuration and retries registration later.

## 17. Location Routing

Data is stored in Google Sheet tabs separated by location.

Runtime routing:
Device ID -> CONFIG -> Location -> Location data tab.

CONFIG is authoritative for routing.

When a device is moved/reconfigured:
- BLE updates device location.
- Apps Script updates CONFIG.
- Subsequent measurements route to the new location.
- If the new location tab does not exist, the script creates it.

Multiple devices at the same location share one location data tab.

Multiple Wi-Fi networks do not affect routing. Devices use their provisioned customer/application SERVER_URL and are identified by Device ID.

Location names must be sanitized before use as Google Sheet tab names to handle invalid characters, duplicates, and tab-name length restrictions.

## 18. Data Tabs

Suggested columns:
- Timestamp
- Device ID
- Device Name
- Battery mV
- Input 1 mV
- Input 2 mV
- Input 3 mV
- Input 4 mV
- Input 1 Value
- Input 2 Value
- Input 3 Value
- Input 4 Value

Additional status/fault fields can be added later.

## 19. EVENTS / Alarm Handling

Central EVENTS tab:
- Timestamp
- Device ID
- Location
- Sensor/Input
- Event
- Value
- Limit

Stateful alarm logic:
NORMAL -> ALARM -> ALARM ACTIVE -> RECOVERED -> NORMAL

This prevents repeated out-of-range readings from generating notification spam.

Events may include:
- LOW
- HIGH
- RECOVERY
- SENSOR FAULT

Paddle inputs can treat expected ON/OFF voltage states normally and an unexpected intermediate voltage as a possible fault/disconnected condition.

Customer-specific Apps Script determines the notification method, such as Discord or email.

## 20. Current BOM / Cost Target

| Item | Estimated cost |
|---|---:|
| 3D-printed case | $1.00 |
| XIAO ESP32-C6 | $5.20 |
| 1S 18650 3500 mAh battery | $4.00 |
| Power switch | $0.25 |
| Four waterproof 3-pin connector/gland assemblies | $12.00 |
| TPS22929 load switch | $0.75 |
| Miscellaneous parts | $3.00 |
| External antenna | $5.00 |
| **Total** | **$31.20** |

Target is approximately $30 for the main device, with sensors priced separately.

The four waterproof field connections are intentionally retained despite their approximately $12 contribution because they make seasonal sensor installation/removal simple and avoid opening the electronics enclosure.

## 21. Initial Deployment / Business Context

Initial customer/application: Chesapeake Bay Foundation (CBF).

Initial requirement:
- 5 monitoring units
- Potentially 8 additional units next year at another location

Longer-term deployment may expand to additional installations, including possible buoy-mounted units in the Chesapeake Bay.

## 22. Design Principles

1. Keep the hardware simple.
2. No custom PCB.
3. Make the electronics tray removable.
4. Keep sensor power switched off during sleep.
5. Keep the battery monitor always connected.
6. Use raw millivolts over the wireless interface.
7. Keep sensor conversion on the server.
8. Use BLE for provisioning/reconfiguration.
9. Do not store Wi-Fi credentials in Google Sheets.
10. Do not use configuration versioning.
11. Check runtime behavior settings on every wake.
12. Cache last known runtime settings for network outages.
13. Make seasonal sensor removal/install possible without opening the electronics enclosure.
14. Keep customer-specific server logic out of generic firmware.
15. Preserve spare GPIO/analog capacity where practical.

## 23. Items Requiring Hardware Validation

The architecture is locked, but these should be validated on the physical prototype before production:
- Actual XIAO boot/reset state of GPIO14/GPIO15.
- RF switch/U.FL behavior and external antenna performance.
- Actual deep-sleep current of the complete assembled node.
- Wi-Fi connection time and current pulse.
- Sensor warm-up/stabilization behavior, especially the 2-second default.
- ADC readings and calibration for each analog input.
- Paddle-switch voltage levels and cable effects.
- TPS22929 sensor-rail behavior at startup/shutdown.
- Battery life using measured rather than estimated current consumption.
- Waterproof gland/connector sealing under actual field conditions.

---

**Status:** Current architecture and field-connection approach are locked as of September 18, 2026.
