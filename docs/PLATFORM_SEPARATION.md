# Krek Labs Platform Separation

**Status:** Current working structure
**Last updated:** 2026-09-24

This document defines what is separated from the Krek Labs Sensor Monitoring System and why.

## 1. Customer Monitoring Portal

**Purpose:** Customer-facing monitoring dashboard.

Target deployment:
- GitHub Pages
- Future custom URL: `https://monitor.kreklabs.com`
- PWA-style web application
- No customer installation required

Contains:
- Login/session UI
- Dashboard
- Locations
- Nodes
- Sensors
- Graphs/history
- Alerts
- Settings
- Customer-facing configuration screens where permitted

Does not contain:
- Customer secrets
- Google Sheet credentials
- Apps Script deployment secrets
- Backend authorization secrets
- Device BLE provisioning logic

The portal communicates with the backend/API layer.

## 2. Technician Installer

**Purpose:** Local equipment installation and provisioning.

Target deployment:
- GitHub Pages
- Future custom URL: `https://install.kreklabs.com`
- Used through Bluefy on iPhone for BLE access

Contains:
- Find Hub
- Find Node
- Identify factory hardware identity
- Assign customer-facing IDs
- Select location
- Select assigned Hub
- Configure sensor ports
- Configure reporting interval
- Configure Node-wide Sensor Delay
- Write persistent configuration
- Read back and verify
- Hardware/sensor test
- Register equipment through the backend

The installer is operationally separate from the customer monitoring portal.

## 3. Node Firmware

Production hardware:
- ESP32-C3 SuperMini

Responsibilities:
- BLE provisioning
- Persistent runtime configuration
- Sensor power control
- Sensor stabilization delay
- Sensor acquisition
- Battery measurement
- ESP-NOW telemetry
- Sleep/wake operation

The firmware does not contain customer-specific alert limits or conversion/calibration logic that can be handled server-side.

## 4. Hub Firmware

Production/bench hardware:
- Seeed XIAO ESP32-C6

Responsibilities:
- BLE provisioning
- Persistent Hub identity/configuration
- ESP-NOW receive
- Wi-Fi connection
- Forward telemetry to Apps Script/API
- Hub status reporting

The Hub has no sensor data of its own.

## 5. Backend / Apps Script

Responsibilities:
- API/data endpoint
- Authentication/authorization integration
- Customer and equipment registration
- Node Config / Hub Config handling
- Telemetry processing
- Sensor conversion
- Alert evaluation
- Alert/event lifecycle
- Google Sheet reads/writes
- Discord notification routing

The backend is the protected logic layer between the public web applications and customer data.

## 6. Google Sheet

Each customer has a separate Sheet.

The Sheet remains the customer-specific data/configuration store and contains:
- Node Config
- Hub Config
- Users / Contacts
- Locations
- Alerts / Events
- Configuration
- One telemetry tab per location

Passwords must never be stored in the Sheet.

## 7. Shared engineering contracts

The following remain in the engineering source-of-truth repository and must be kept synchronized:
- ESP-NOW packet format
- BLE provisioning protocol
- Node persistent configuration schema
- Hub persistent configuration schema
- Node Config sheet schema
- Hub Config sheet schema
- API/backend contract
- Alert model
- Sensor type definitions
- Port mode definitions
- Sensor Delay definition

## 8. Deployment separation

The preferred eventual deployment separation is:

- `krek-monitor` GitHub repository -> `monitor.kreklabs.com`
- `krek-install` GitHub repository -> `install.kreklabs.com`
- `LR-SiFi` remains the engineering/source-of-truth repository

Until the deployment repositories are created, implementation can be staged under `web/monitor/` and `web/install/` in LR-SiFi without exposing any secrets.

## 9. Security boundary

GitHub Pages applications are public frontend code.

Never place these in GitHub Pages:
- Passwords
- API secrets
- Customer Sheet credentials
- Private access tokens
- Backend secrets

Public frontend -> authenticated/protected backend -> customer data.

## 10. Cost target

The architecture is intentionally designed around:
- GitHub Pages for frontend hosting
- Google Apps Script for backend/data processing
- Google Sheets for customer data
- Discord webhooks for notifications
- No required customer software subscription
- No required rented VPS/server

The domain `kreklabs.com` provides professional URLs but is not itself a server.
