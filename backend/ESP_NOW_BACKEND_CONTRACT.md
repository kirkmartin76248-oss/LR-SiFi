# Backend / Hub Contract

## Hub telemetry request

Logical request fields:
- op: telemetry
- hubId
- hubMac
- nodeId
- sequence
- configRevision
- receivedAt
- rssi
- batteryMv
- values[4]
- presentMask

The exact HTTP encoding can change without changing the device protocol.

## Backend processing

1. Authenticate and validate the Hub request.
2. Identify customer and Hub.
3. Identify Node.
4. Read authoritative Node Config.
5. Confirm Node belongs to the Hub/location.
6. Convert analog values using Node Config.
7. Interpret normalized digital values.
8. Evaluate configured limits.
9. Update alert/event state.
10. Route immediate alerts to the location ALERT_WEBHOOK.
11. Write raw/converted telemetry to the location data tab.
12. Return any newly generated Node configuration revision to the Hub for pending delivery.

## Configuration response

Logical response fields:
- op: pendingUpdate
- nodeId
- configRevision
- config:
  - hubId
  - hubMac
  - reportingIntervalS
  - sensorDelayMs
  - ports[4]
    - enabled
    - sensorType
    - mode
    - activeLevel

The Hub stores this update in its pending mailbox. It does not wait for or contact the Node as part of the backend transaction.

## Backend-to-Hub requirements

- Backend updates are idempotent by nodeId + configRevision.
- Hub retries failed requests.
- Hub must not erase a pending Node update until CONFIG_ACK is received from that Node.
- Backend remains authoritative if Hub and backend disagree.
