# WiFi William / Krek Labs Sensor Monitoring System — Current Architecture

**Status:** Current working specification  
**Last updated:** 2026-09-24  
**Company:** Krek Labs, LLC

This document supersedes earlier assumptions where they conflict. The GitHub repository is the engineering source of truth for the firmware, Apps Script, sheet schema, and mobile-app architecture.

## 1. Product and network architecture

Current product name in the customer app: **Krek Labs Sensor Monitoring System**.

Current wireless architecture is **ESP-NOW from Node to Hub**. LoRa is not part of the current system.

Runtime path:

Node -> ESP-NOW -> Hub -> Wi-Fi -> customer-specific Google Apps Script -> customer's Google Sheet -> Krek Labs mobile app

BLE is used for local installation/provisioning/configuration of Hub and Node.

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

## 5. Node Config is authoritative for node-specific alert limits

Do **not** create a separate alert-rule table for every ordinary node sensor limit if those limits naturally belong to the node configuration.

Node Config contains, for each configured sensor:
- Sensor type
- Enabled/disabled state as appropriate
- Conversion parameters as needed
- Low alert limit
- High alert limit
- Any sensor-specific alarm behavior

Example conceptual fields:

Node ID | Location | Hub ID | Sleep Interval | Sensor 1 Type | S1 Low | S1 High | Sensor 2 Type | S2 Low | S2 High | Sensor 3 Type | S3 Low | S3 High | Sensor 4 Type | S4 Low | S4 High

This keeps the configuration and its alarm limits together and makes the app's Node Configuration screen map naturally to the sheet.

The Apps Script reads the node's limits when processing measurements.

## 6. Sensor data

The Node should continue to send raw measurement values in the compact device payload wherever practical.

For the current water-monitoring design:
- Analog sensors are transmitted as millivolts
- Digital/thresholded switch behavior can be represented according to the final firmware protocol
- Battery is transmitted as millivolts

The Apps Script performs sensor conversion using the Node Config sensor type and parameters.

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

## 8. Notifications

Notifications must work even when the Krek Labs app is closed or the customer is not currently logged in.

Email:
- Apps Script/backend can send email to authorized customer contacts based on the customer's Users/Contacts and notification settings.

Push:
- The mobile app registers each authorized user's device with the notification service.
- Push delivery is independent of the app being open.
- Logging out should not automatically mean the person stops receiving alerts unless the user disables notifications or the account/device is deactivated.
- Tapping a push notification should deep-link to the relevant location/node/sensor/alert in the app.

A user can have access to some locations but not others, so notification routing must respect location permissions.

## 9. Locations

A customer can have multiple locations in the same customer account and Google Sheet.

A location contains:
- Location ID
- Name
- Description/metadata
- One or more hubs
- One or more nodes
- One location-specific telemetry tab

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

App:
1. Select customer location.
2. Find Node over BLE.
3. Identify/register hardware.
4. Assign Node number/ID.
5. Assign Hub.
6. Configure four sensor ports.
7. Configure reporting/sleep interval.
8. Configure any other supported Node settings.
9. Run sensor/device test.
10. Register/update Node Config in the customer's sheet.
11. Ask: **Install another node?**
    - Install Another Node
    - Continue

The app must not require the customer to re-enter the Google Apps Script URL during Node installation.

## 12. Hub installation flow

App:
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
6. If continuing, create first location and install equipment.
7. If skipping, enter the normal app with an empty system and return to setup later.

## 14. App structure

Primary app areas:
- Dashboard
- Locations
- Alerts
- Settings

Installation is available as a prominent action and can be started from the appropriate area.

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

## 16. Change-control rule

When a design decision changes:
1. Update this architecture document first.
2. Update the Google Sheet schema/Apps Script contract.
3. Update firmware protocol/configuration as required.
4. Update mobile app behavior.
5. Keep the GitHub repository as the shared source of truth.

Do not silently change one layer without checking the other layers.

## 17. Legacy code note

The existing repository files named Google Script, Node, and Gateway were written under an earlier architecture. They should be treated as implementation starting points/reference material, not as the final specification.

In particular, the current Google Script uses a single Config sheet and creates location tabs dynamically, which is directionally useful. The new design retains the per-location data-tab approach but changes the configuration model to separate Node Config and Hub Config and moves node-specific alert limits into Node Config.

