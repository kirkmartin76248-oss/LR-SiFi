# Google / Discord Backend — Locked

Relay uploads JSON over HTTPS to one Google Apps Script endpoint.

Google is the configuration and interpretation master.

The relay remains operational if Internet is unavailable. It buffers raw measurements and uploads backlog after connectivity returns.

Duplicate protection:
- requestId for Relay ↔ Google transaction
- Node MAC + Sequence for measurement identity

Unknown valid nodes:
- store in RAW_DATA
- mark unregistered/unknown
- do not route to site DATA
- do not trigger sensor alarms

## Discord
Two webhooks per location:
1. Alerts
2. Monthly Status

Alert behavior:
- Normal → Out of Tolerance: one alert
- Persistent condition: no repeated alerts
- Return to normal: one recovery
- Later recurrence: new alert

Monthly status includes communications, battery, temperature, DO, water flow, air flow, events and device health.
