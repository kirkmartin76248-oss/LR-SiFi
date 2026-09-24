# ESP-NOW Production Protocol

Status: locked working contract.

## Roles

- Node: sleeps, measures, sends telemetry, receives configuration updates.
- Hub: always-on ESP-NOW receiver/bridge. ACKs telemetry immediately and maintains only a pending-update mailbox keyed by Node ID.
- Backend: authoritative configuration source. It never participates in the Node wake transaction directly.

The Node never waits for Google Apps Script, Wi-Fi, or the backend.

## Message sequence

1. Node wakes and initializes ESP-NOW.
2. Node sends TELEMETRY.
3. Hub immediately sends TELEMETRY_ACK.
4. Node sends UPDATE_QUERY.
5. Hub checks its pending-update mailbox for that Node ID.
6. Hub sends either NO_UPDATE or CONFIG_UPDATE.
7. If a configuration update is sent, Node validates, stores it persistently, and sends CONFIG_ACK.
8. Node powers down sensors/radio and enters sleep.
9. Independently, the Hub forwards the received telemetry to the backend with Hub UTC timestamp and Hub-measured RSSI. If Wi-Fi/backend is unavailable, the Hub stores the record in persistent offline telemetry storage and uploads it later with the original timestamp.
10. Backend may later place a new configuration update into the Hub's pending-update mailbox.

## Packet rules

- ESP-NOW payloads use a compact binary format.
- All multi-byte integers are little-endian.
- Maximum application payload is kept below the ESP-NOW 250-byte payload limit.
- protocol_version rejects incompatible packets.
- message_id identifies the transaction.
- Node sequence increments for each telemetry report.
- Hub RSSI is measured at the Hub and is not supplied by the Node.
- The Node includes a 4-byte Config Fingerprint in every telemetry packet.
- The Hub ACK includes the current/pending 4-byte Config Fingerprint and a Config Changed flag.
- Configuration updates carry a monotonically increasing config_revision and the new 4-byte Config Fingerprint.
- The Config Fingerprint is automatic; customers do not enter or manage it.
- A Node only applies a configuration update when its revision is newer than the stored revision.
- Telemetry ACK and CONFIG ACK are different messages.

## Message types

| Value | Message |
|---:|---|
| 1 | TELEMETRY |
| 2 | TELEMETRY_ACK |
| 3 | UPDATE_QUERY |
| 4 | NO_UPDATE |
| 5 | CONFIG_UPDATE |
| 6 | CONFIG_ACK |

## Telemetry

Telemetry contains:
- node_id
- sequence
- config_revision
- node_uptime_seconds
- battery_mv
- 4-byte config_fingerprint
- four raw sensor-port values
- enabled/present mask

Analog values are millivolts. Digital values are normalized 0=inactive, 1=active.

The Hub adds:
- received_at_utc_ms timestamp from the NTP-synchronized Hub clock
- Hub RSSI
- Hub ID
- location/backend identity as configured

If Wi-Fi is offline, the Hub stores the complete backend record locally and later forwards it without changing the original receive timestamp.

Sensor conversion and alert evaluation remain server-side.

## Configuration update

A configuration update contains only device-executable configuration:
- config_revision
- config_fingerprint
- node_id
- hub_id
- hub_mac
- reporting_interval_seconds
- sensor_delay_ms
- four port configurations: enabled, sensor_type, signal_mode, digital active level

Customer alert thresholds and analog conversion/calibration parameters remain server-side.

## Reliability

The Node retries telemetry if no ACK is received within the firmware timeout. Retries use the same sequence number.

The Hub must ACK a duplicate telemetry packet again, but must not create duplicate backend telemetry for the same node_id and sequence.

Configuration updates are retained in the pending mailbox until the Node acknowledges the matching revision.

Pending configuration updates should survive Hub restart using Hub nonvolatile storage. This mailbox is not a Node database; it contains only updates awaiting delivery.

## Timing

The Hub maintains UTC time using NTP (default server: `pool.ntp.org`). Node telemetry is timestamped by the Hub at ESP-NOW receipt time, not when the record eventually reaches Google Apps Script. Offline records retain that original timestamp.

The protocol intentionally avoids a long Hub polling/preamble cycle.

The Hub is always listening. The Node wakes, transmits a short packet, waits for the immediate ACK, performs the update query, then sleeps.

Exact retry/response timing constants are firmware constants and should be tuned on hardware rather than encoded as customer configuration.
