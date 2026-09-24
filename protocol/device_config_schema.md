# Device Configuration Schema

## Node persistent configuration

All production Nodes use the same firmware image.

| Field | Type | Notes |
|---|---|---|
| node_id | string[16] | Customer-assigned ID |
| hub_id | string[16] | Assigned Hub reference |
| hub_mac | uint8[6] | Actual ESP-NOW MAC read from Hub |
| reporting_interval_s | uint32 | 300 to 43200 seconds |
| sensor_delay_ms | uint32 | One Node-wide stabilization delay |
| config_revision | uint32 | Monotonic backend revision |
| port[0..3] | PortConfig | Physical sensor configuration |

### PortConfig

| Field | Type | Meaning |
|---|---|---|
| enabled | bool | Port is used |
| sensor_type | uint8 | Metadata/type identifier |
| signal_mode | uint8 | ANALOG or DIGITAL |
| active_level | uint8 | HIGH or LOW for digital ports |

Firmware behavior:
- ANALOG: ADC input, no digital pull-up/down, report millivolts.
- DIGITAL + HIGH: internal pull-down, GPIO HIGH means logical active.
- DIGITAL + LOW: internal pull-up, GPIO LOW means logical active.
- Disabled: do not sample/report as an active sensor.

## Hub persistent configuration

| Field | Type | Notes |
|---|---|---|
| hub_id | string[16] | Customer-assigned Hub ID |
| hub_mac | uint8[6] | Physical ESP-NOW MAC |
| location_id | string[16] | Assigned location |
| wifi_ssid | string | Provisioned Wi-Fi |
| wifi_password | string | Stored only on Hub |
| backend_url | string | Customer Apps Script/backend URL |
| firmware_version | string | Reported status |
| config_revision | uint32 | Hub configuration revision |

The Hub does not persist a Node list.

## Pending-update mailbox

Each mailbox entry contains:
- node_id
- config_revision
- serialized Node configuration
- pending/acknowledged state
- created/updated timestamp

Mailbox entries are retained until the Node sends CONFIG_ACK for the matching revision.

The mailbox is communication state, not authoritative configuration. Backend/Node Config remains authoritative.

## Sensor type identifiers

Initial identifiers:
- 0 = DISABLED
- 1 = WATER_TEMPERATURE
- 2 = DISSOLVED_OXYGEN
- 3 = WATER_FLOW
- 4 = AIR_FLOW
- 255 = OTHER

Additional sensor types may be added without changing the ANALOG/DIGITAL physical interface contract.
