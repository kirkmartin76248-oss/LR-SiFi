# Krek Labs Installer

Technician-facing BLE installation/configuration web application.

Target URL:

`https://install.kreklabs.com`

Hosting target: GitHub Pages.

Primary workflow:

Configure -> Write to Node/Hub -> Read Back/Verify -> Register -> Test

The installer is intended to be opened through a BLE-capable browser such as Bluefy on iPhone.

Node configuration includes:
- Node ID
- Assigned Hub
- Hub ESP-NOW MAC
- Per-port sensor type
- Per-port ANALOG/DIGITAL mode
- Digital Active Level
- Reporting interval
- Node-wide Sensor Delay

The installer must never require manual entry of a Hub MAC. It is obtained from the registered Hub configuration.
