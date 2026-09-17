# Water Sensor Hub — Rev 1

Target hardware:
- Seeed Studio XIAO ESP32-C6
- Seeed Wio-SX1262 868–930 MHz module, used at 915 MHz
- Arduino IDE
- RadioLib

## Locked baseline
- US915
- 915 MHz initial frequency
- SF7
- BW 125 kHz
- CR 4/5
- TX power 14 dBm initially
- Hub-master sequential polling
- Hub uses a long ~1.5 s preamble
- Node receiver baseline: 20 ms RX / ~1 s sleep
- Raw sensor values are transmitted as millivolts
- Digital switches are transmitted as 0/1

## XIAO / Wio-SX1262 pins
D3 / GPIO21  -> SX1262 NSS
D5 / GPIO23  -> SX1262 RF_SW
D6 / GPIO16  -> SX1262 RESET
D7 / GPIO17  -> SX1262 BUSY
D8 / GPIO19  -> SX1262 SCK
D9 / GPIO20  -> SX1262 MISO
D10 / GPIO18 -> SX1262 MOSI
MTDO / GPIO7 -> reserved / node DIO1 wake pin (hub does not need it for normal polling)

## Current status
This is a Rev 1 hub software framework, not a production firmware drop.
The two areas intentionally isolated for the next revision are:
1. Exact Wio-SX1262 RF-switch control for the installed Seeed hardware.
2. Wi-Fi provisioning + Google Apps Script configuration synchronization.

The LoRa payload parser already follows the project's raw-mV/0-1 telemetry rule.
