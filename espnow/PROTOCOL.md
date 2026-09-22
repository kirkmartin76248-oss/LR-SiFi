# ESP-NOW Protocol v1.0 — Locked

Little-endian multi-byte integers. ESP-NOW unicast. ESP-NOW Long Range mode.

## Node → Relay MEASUREMENT — 24 bytes
| Offset | Size | Field |
|---|---:|---|
| 0 | 1 | Packet Type = 0x01 |
| 1 | 1 | Protocol Version = 0x01 |
| 2–7 | 6 | Node MAC |
| 8–9 | 2 | Sequence |
| 10–11 | 2 | Sensor 1 raw mV |
| 12–13 | 2 | Sensor 2 raw mV |
| 14 | 1 | Sensor 3 raw 0/1 |
| 15 | 1 | Sensor 4 raw 0/1 |
| 16–17 | 2 | Battery mV |
| 18–21 | 4 | Config Fingerprint |
| 22–23 | 2 | Reserved |

## Relay → Node ACK — 5 bytes
Packet Type 0x02, protocol version, sequence, status.
- 0x00 ACCEPTED
- 0x01 DUPLICATE
- 0x02 INVALID

ACK is sent immediately by the relay and never waits for Google.

## Configuration exchange
CONFIG_CHECK = 0x03, 8 bytes.
CONFIG_NONE = 0x04, 8 bytes.
CONFIG_AVAILABLE = 0x05, 12 bytes.
CONFIG_REQUEST = 0x06, 12 bytes.
CONFIG_DATA = 0x07, 19 bytes:
Node MAC + fingerprint + Sleep Interval uint32 seconds + Sensor Delay uint16 seconds + RF Power int8.
CONFIG_SAVED = 0x08, 12 bytes.

Pending configuration is deleted only after validated CONFIG_SAVED.

## Cloud JSON
Relay → Google:
requestId, relay {mac, firmware}, measurements[].

Each measurement includes:
nodeMac, sequence, timestamp, sensor1Raw, sensor2Raw, sensor3Raw, sensor4Raw, batteryMv, rssi, configFingerprint.

Google → Relay:
requestId, status, accepted[], updates[].

Google returns only operational node updates:
nodeMac, sleepInterval, sensorDelay, rfPower, configFingerprint.
