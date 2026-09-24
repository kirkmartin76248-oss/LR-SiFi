# WiFi William / Krek Labs Sensor Monitoring System — Current Architecture

**Status:** Current working specification  
**Last updated:** 2026-09-24

**Recent architecture change:** Node sensor ports are now configured as Analog or Digital in firmware. Digital ports also store an Active Level and use the corresponding internal pull resistor.  
**Company:** Krek Labs, LLC

This document supersedes earlier assumptions where they conflict. The GitHub repository is the engineering source of truth for the firmware, Apps Script, sheet schema, and mobile-app architecture.

## 1. Product and network architecture

Current product name in the customer app: **Krek Labs Sensor Monitoring System**.

Current wireless architecture is **ESP-NOW from Node to Hub**. LoRa is not part of the current system.

Runtime path:

Node -> ESP-NOW -> Hub -> Wi-Fi -> customer-specific Google Apps Script/backend -> customer's Google Sheet -> Krek Labs Monitoring Portal (web/PWA)

BLE is used for local installation/provisioning/configuration of Hub and Node through the separate Krek Labs Installer web application, intended to be opened in a BLE-capable browser such as Bluefy on iPhone.

The customer Monitoring Portal and Technician Installer are separate frontend applications. The preferred production deployment is separate GitHub Pages repositories with custom subdomains:
- `monitor.kreklabs.com` -> Monitoring Portal
- `install.kreklabs.com` -> Technician Installer

GitHub Pages hosts frontend code only. Customer data, secrets, authorization, telemetry processing, alert evaluation, and Google Sheet access remain in the backend/API layer.

The Hub has no sensors. All sensor measurements belong to Nodes.

## 2. Customer/account model

Each customer has:
- One Krek Labs customer account
- One customer-specific Google Sheet
- One customer-configured Google Apps Script URL/data endpoint
- One or more locations
- One or more hubs
- One or more nodes
- Multiple authorized users

The Google Apps Script URL is entered once during customer setup and is reused for Hub installation and the customer's system. The Hub does not require the customer to re-enter the URL.

The app is cloud-connected. Each person gets an individual login. Credentials are never shared between team members.

Authentication/passwords are handled by the app/backend authentication layer, not stored as passwords in Google Sheets.

## 3. Customer team and permissions

A customer can have multiple users, for example:
- Administrator
- Technician
- Manager
- Viewer

Each user has:
- Individual account/login
- Customer membership
- Role
- Location access
- Notification preferences

The customer-facing Google Sheet may contain a **Users** or **Contacts** tab for operational contact information and email routing, but it must not contain passwords.

The app/backend remains authoritative for authentication and authorization. The Apps Script uses the customer sheet's user/contact and notification data to determine email recipients where applicable.

Push-notification device tokens should be managed by the notification/backend service rather than exposed as ordinary sheet data.

## 4. Customer Google Sheet strategy

Each customer gets a separate Google Sheet.

The sheet is intentionally organized so high-volume telemetry is split by location rather than putting every location into one giant data tab. This reduces growth pressure from Google Sheets row limits and makes location-level data management simpler.

### Core tabs

**Node Config**
- Node identity and configuration
- Location assignment
- Hub assignment
- Sensor assignments
- Runtime settings
- Alert limits for the sensors on that node

**Hub Config**
- Hub identity
- Location assignment
- Wi-Fi/connection status information that is appropriate to expose operationally
- Firmware/version/status information
- Google endpoint association as appropriate

**Users / Contacts**
- Customer team members
- Email addresses
- Role
- Active/inactive status
- Notification preferences/routing

**Locations**
- Location ID
- Location name
- Description
- Status/metadata
- Data-tab name/identifier
- Discord Alert Webhook
- Discord Monthly Status Webhook

**Alerts / Events**
- Alert and recovery history
- Timestamp
- Location
- Node
- Sensor
- Event type
- Value
- Limit
- Notification state/status

**Configuration**
- Customer/system-wide settings that are not node-specific

### Location data tabs

Each location gets its own telemetry data tab.

Example:
- Annapolis
- Cambridge
- Research_Tank_01

Multiple nodes at the same location write to the same location data tab.

The Apps Script determines the destination using:

Node ID -> Node Config -> Location -> location data tab

If a new location is registered and its data tab does not exist, the script creates it.

Location tab names must be sanitized for Google Sheets naming restrictions and collisions.

## 5. Node sensor-port configuration and Node Config authority

Do **not** create a separate alert-rule table for every ordinary node sensor limit if those limits naturally belong to the node configuration.

Node Config contains, for each configured sensor port:
- Enabled/disabled state
- Signal/input mode: `ANALOG` or `DIGITAL`
- Sensor type
- For Analog: conversion parameters as needed
- For Digital: Active Level (`HIGH` or `LOW`)
- Low/high alert limits when applicable
- Any sensor-specific alarm behavior

Firmware uses the port's signal/input mode to configure the physical GPIO/ADC behavior. Analog means ADC input and raw millivolt reporting. Digital means digital input plus the configured internal pull resistor. The Active Level determines which physical GPIO level means logical `active`; firmware reports the normalized logical state as `1` active or `0` inactive.

Example conceptual fields:

Node ID | Location | Hub ID | Sleep Interval | Sensor 1 Type | S1 Mode | S1 Active Level | S1 Low | S1 High | Sensor 2 Type | S2 Mode | S2 Active Level | S2 Low | S2 High | Sensor 3 Type | S3 Mode | S3 Active Level | S3 Low | S3 High | Sensor 4 Type | S4 Mode | S4 Active Level | S4 Low | S4 High

This keeps the configuration and its alarm limits together and makes the app's Node Configuration screen map naturally to the sheet.

The Apps Script reads the node's limits when processing measurements.

## 6. Sensor data

The Node should continue to send raw measurement values in the compact device payload wherever practical.

For the current water-monitoring design:
- Analog sensors are transmitted as millivolts.
- Digital sensors are configured in firmware as Digital inputs with an Active Level of HIGH or LOW.
- For a Digital port, firmware configures the internal pull resistor opposite the active level: Active HIGH -> internal pull-down; Active LOW -> internal pull-up.
- Firmware reports Digital state logically as `1 = active` and `0 = inactive`, so a water-flow/pressure switch can be wired either active-high or active-low without changing the application protocol.
- Analog ports use the ADC and do not enable a digital pull-up/pull-down.
- Battery is transmitted as millivolts.

The Apps Script performs analog sensor conversion using the Node Config sensor type and parameters. Digital interpretation does not require server-side inversion because firmware normalizes the configured active level to logical active/inactive state, although Node Config retains the Active Level for configuration, display, diagnostics, and re-provisioning.

This keeps conversion/calibration logic server-side and allows configuration changes without reflashing field devices.

Location data tabs should preserve raw values and may also contain converted/display values when useful for the app.

## 7. Alerts

Alert evaluation occurs server-side.

The ESP32 does not need to receive alert thresholds merely to decide whether an alarm exists.

Processing flow:

Measurement arrives
-> identify Node
-> read Node Config
-> convert value if required
-> compare with node-specific low/high limits
-> update alert/event state
-> notify configured recipients
-> write telemetry to the appropriate location tab

Stateful alert handling should prevent notification spam.

Example state progression:

NORMAL -> ALERT ACTIVE -> RECOVERED -> NORMAL

A persistent out-of-range condition should not generate a new email/push notification on every measurement unless the customer explicitly configures that behavior.

## 8. Discord notifications and monthly status reports

The current notification architecture uses Discord webhooks rather than a dedicated Krek Labs push-notification service.

Each customer **location** has two separate Discord webhooks:

- `ALERT_WEBHOOK` — immediate/actionable alerts for that location.
- `REPORT_WEBHOOK` — scheduled monthly status reports/summaries for that location.

These are location-specific, not customer-wide. Multiple Nodes at the same location share that location's alert channel and monthly report channel.

When a customer creates a new location, the location-creation flow must ask for:
1. Discord Alert Webhook
2. Discord Monthly Status Webhook

The customer can enter/configure these during location creation and may update them later through authorized location configuration. The webhook URLs are stored server-side/customer configuration and are never written to ESP32 firmware or exposed in Node telemetry.

Routing is:

**Node ID -> Node Config -> Location -> location webhook configuration**

### Alert webhook behavior

`ALERT_WEBHOOK` handles immediate/actionable events. Stateful alert handling should:
- Send once when a condition becomes active/out of tolerance.
- Suppress repeated notifications while the same condition remains active.
- Send one recovery/`NORMAL` notification when the condition returns to normal.
- Send a new alert if the condition later recurs.

This prevents notification spam while preserving the alert lifecycle.

### Monthly status webhook behavior

`REPORT_WEBHOOK` receives the scheduled monthly status report for that location.

The monthly report is a detailed operational summary and should include, as applicable:
- Concise overall location/system summary
- Per-device communication/status information
- Battery status
- Water temperature
- Dissolved oxygen
- Water-flow status/duration as applicable
- Air-flow status/duration as applicable
- Alert/event history for the reporting period
- Device health/status information

The report is separate from immediate alerts so routine reporting does not interfere with the actionable alert channel.

Discord credentials/webhook URLs remain server-side and are never stored in ESP32 firmware.

A user can have access to some locations but not others, so the Monitoring Portal must respect location permissions when displaying alert/report configuration or history.

## 9. Locations

A customer can have multiple locations in the same customer account and Google Sheet.

A location contains:
- Location ID
- Name
- Description/metadata
- One or more hubs
- One or more nodes
- One location-specific telemetry tab
- One Discord Alert Webhook
- One Discord Monthly Status Webhook

When a new location is created, the customer/authorized installer is prompted for both Discord webhook URLs as part of the location setup. The location is not required to have a webhook if the customer's operational design intentionally leaves that channel unconfigured, but the setup flow must explicitly present both fields rather than assuming a customer-wide webhook.

The Apps Script uses the location's webhook configuration when routing alerts and monthly status reports.

The Hub itself has no sensor measurements.

Location dashboards aggregate or display measurements from the Nodes assigned to that location.

## 10. Hub

Hub responsibilities:
- Receive Node telemetry over ESP-NOW
- Maintain ESP-NOW communication with Nodes
- Connect to customer Wi-Fi
- Forward Node telemetry to the customer's Apps Script
- Receive server responses/runtime information as required
- Report Hub status

Hub dashboard information can include:
- Online/offline
- Last communication
- Wi-Fi status
- Firmware
- ESP-NOW status
- Number of connected/known Nodes
- Configuration status

Do not display temperature, dissolved oxygen, flow, or other sensor measurements as if they were Hub measurements.

## 11. Node installation flow

Installer web application:
1. Select customer location.
2. Find Node over BLE.
3. Identify/register hardware.
4. Assign Node number/ID.
5. Assign Hub.
6. Configure four sensor ports.
   - Select sensor type or Disabled.
   - Firmware signal mode is derived as `ANALOG` or `DIGITAL` from the selected sensor configuration.
   - For Digital sensors, select Active Level: HIGH or LOW.
   - Firmware uses Active HIGH -> pull-down or Active LOW -> pull-up.
7. Configure reporting/sleep interval.
8. Configure the Node-wide Sensor Delay. This is one value for the Node and is selected to satisfy the sensor with the longest required stabilization time.
9. Configure any other supported Node settings.
10. Write configuration to persistent Node memory.
11. Read back and verify Node ID, Hub MAC, sensor-port configuration, Sensor Delay, reporting interval, and other applicable settings.
12. Run sensor/device test.
13. Register/update Node Config in the customer's sheet.
14. Ask: **Install another node?**
    - Install Another Node
    - Continue

The app must not require the customer to re-enter the Google Apps Script URL during Node installation.

## 12. Hub installation flow

Installer web application:
1. Select customer location.
2. Find Hub over BLE.
3. Configure Wi-Fi.
4. Use the customer's already-registered Google Apps Script URL.
5. Register/update Hub Config.
6. Test Wi-Fi/data connection.
7. Finish.

## 13. Customer onboarding

1. Create Krek Labs account.
2. Enter customer information.
3. Enter Google Apps Script URL.
4. Test connection.
5. Choose:
   - Continue setup
   - Skip setup for now
6. If continuing, create first location. During location creation, collect the location's Discord Alert Webhook and Discord Monthly Status Webhook, then install equipment.
7. If skipping, enter the normal app with an empty system and return to setup later.

## 14. Web application structure

There are two separate frontend applications.

### Krek Labs Monitoring Portal

Customer-facing web/PWA application hosted on GitHub Pages.

Primary areas:
- Dashboard
- Locations
- Alerts
- Settings

It is designed for PC, Mac, iPhone, Android, and tablet browsers. No app-store installation is required.

### Krek Labs Installer

Technician-facing web application hosted on GitHub Pages and opened through a BLE-capable browser such as Bluefy on iPhone.

It handles Hub and Node discovery, provisioning, read-back verification, registration, and installation tests.

Installation is therefore separated from normal customer monitoring rather than being embedded in the customer dashboard.

Dashboard:
- Overall system status
- Locations
- Active alerts
- High-level node status

Location:
- Hub status
- Node list
- Node-derived current values
- Location-level graphs based on selected/all nodes
- Location alert/report configuration status

Node:
- Current sensor values
- Battery
- Last update
- Graphs
- Configuration
- Device status

Hub:
- Connectivity/status information only; no sensor values.

## 15. Current engineering repository status

The repository currently contains earlier firmware and Apps Script code that still includes legacy assumptions such as LoRa and the older EcoSYNC naming. Those files are not automatically treated as current architecture.

Before production implementation, the code must be reconciled with this document:
- Node firmware -> ESP-NOW
- Hub firmware -> ESP-NOW + Wi-Fi
- Krek Labs naming
- Node Config schema
- Hub Config schema
- per-location telemetry tabs
- server-side alert limits from Node Config
- multi-user/customer model
- mobile app/API interface

## 16. Node configuration write/verify contract

Node configuration is not only a backend registration. During BLE installation the app writes the selected configuration into persistent Node memory and then reads it back before registration.

At minimum, persistent Node configuration includes:
- Customer-assigned Node ID
- Assigned Hub ID/reference
- Assigned Hub ESP-NOW MAC address
- Per-port sensor type
- Per-port signal mode (`ANALOG` or `DIGITAL`)
- Per-port Digital Active Level when applicable
- Reporting/sleep interval
- Node-wide Sensor Delay
- Other supported runtime settings

The installation contract is:

**Configure -> Write to Node -> Read Back/Verify -> Register -> Test**

The backend Node Config and physical Node configuration must agree. Replacement Nodes use the universal firmware/default identity and are provisioned with the customer's Node ID and Hub MAC during installation.

## 17. Frontend/backend separation and security

The Monitoring Portal and Installer are public frontend applications and must not contain secrets.

GitHub Pages code must never contain:
- Customer passwords
- Google Sheet credentials
- Backend secrets
- Private access tokens

The public frontend communicates with the protected backend/API, which enforces authorization before accessing customer data or performing configuration operations.

The Installer may use BLE locally to communicate with hardware, but device registration and customer data operations still pass through the backend contract.

## 18. Change-control rule

When a design decision changes:
1. Update this architecture document first.
2. Update the Google Sheet schema/Apps Script contract.
3. Update firmware protocol/configuration as required.
4. Update mobile app behavior.
5. Keep the GitHub repository as the shared source of truth.

Do not silently change one layer without checking the other layers.

## 19. Legacy code note

The existing repository files named Google Script, Node, and Gateway were written under an earlier architecture. They should be treated as implementation starting points/reference material, not as the final specification.

In particular, the current Google Script uses a single Config sheet and creates location tabs dynamically, which is directionally useful. The new design retains the per-location data-tab approach but changes the configuration model to separate Node Config and Hub Config and moves node-specific alert limits into Node Config.

