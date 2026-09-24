# Production Hub Firmware Design

Hardware: Seeed XIAO ESP32-C6.

## Startup state machine

BOOT -> LOAD_CONFIG -> RF_SWITCH -> BLE_WINDOW -> WIFI -> ESPNOW -> RUN

### RF switch initialization

The onboard FM8625H RF switch must be explicitly configured before wireless initialization.

Locked XIAO V1.0 mapping:
- GPIO3 = RF switch power: LOW ON, HIGH OFF
- GPIO14 = RF port select: LOW RF1, HIGH RF2
- RF2 = U.FL/external antenna path

Startup sequence:
1. GPIO3 OUTPUT, default HIGH while configuring.
2. GPIO14 OUTPUT, set HIGH.
3. GPIO3 LOW to enable the switch.
4. Initialize Wi-Fi/ESP-NOW.

Do not depend on reset defaults.

## BLE provisioning

A short provisioning window is available at startup when installation/configuration mode is requested.

BLE writes Hub configuration and reads it back for verification.

The Hub reports its actual ESP-NOW MAC over BLE. The Installer uses that value automatically when provisioning Nodes.

## Runtime loop

The Hub runtime has independent services:
- ESP-NOW receive service
- telemetry ACK service
- pending-update mailbox service
- Wi-Fi service
- backend upload queue
- BLE service when enabled
- health/status service

ESP-NOW receive handling must remain responsive even when Wi-Fi/backend operations are slow.

## Telemetry path

ESP-NOW receive -> validate -> immediate ACK -> deduplicate -> enqueue backend record

Backend record includes:
- node_id
- sequence
- config_revision
- raw sensor values
- battery_mv
- hub_id
- received timestamp
- Hub RSSI

The receive path must not block on HTTPS or Apps Script.

## Backend failure

If Wi-Fi/backend is unavailable, the Hub should retain a bounded telemetry queue and retry asynchronously. The Node still receives its immediate ACK and completes its sleep cycle.

Queue capacity and retention policy are firmware implementation parameters to be tuned during bench testing.

## Pending configuration

backend -> Hub -> pending mailbox[node_id]

The Hub does not push the update immediately and does not poll the Node. The update is delivered on the Node's next wake.

When the Node sends UPDATE_QUERY, the Hub responds from the mailbox.

After CONFIG_ACK, the matching mailbox entry is removed/marked delivered.

## Important non-responsibilities

The Hub does not:
- maintain a Node list
- evaluate sensor limits
- convert sensor values
- decide alerts
- contact Google during the Node wake transaction
- require a Node to stay awake for Internet access
