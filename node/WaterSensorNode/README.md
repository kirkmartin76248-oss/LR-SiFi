# Water Sensor Node — Rev 2

Target: Seeed XIAO ESP32-C6 + Wio-SX1262.

## Pin assignment
D0/GPIO0 TEMP ADC
D1/GPIO1 DO ADC
D2/GPIO2 BAT ADC
D3/GPIO21 SX1262 NSS
D4/GPIO22 spare
D5/GPIO23 SX1262 RF_SW
D6/GPIO16 SX1262 NRST
D7/GPIO17 SX1262 BUSY
D8/GPIO19 SX1262 SCK
D9/GPIO20 SX1262 MISO
D10/GPIO18 SX1262 MOSI
MTMS/GPIO4 TPS22929D ON
MTDI/GPIO5 water switch
MTCK/GPIO6 air switch
MTDO/GPIO7 SX1262 DIO1 wake
GPIO15 onboard LED

## Locked baseline
US915, 915 MHz, SF7, BW125 kHz, CR4/5, 14 dBm initial TX power.
Node radio duty cycle: 20 ms RX / 1 s sleep. Hub preamble: 1465 symbols (~1.5 s).

## Payload rule
Analog values are raw millivolts: temperature, dissolved oxygen, battery.
Digital values are 0/1: water flow, air flow. Engineering conversion,
calibration, compensation and alarms remain downstream in Google Apps Script.

## Power
1 x 3300 mAh 18650 to XIAO BAT pads. XIAO onboard regulator supplies 3.3 V.
TPS22929D switches the sensor rail. Battery monitor is 1 MOhm / 1 MOhm divider
with 100 nF capacitor to D2/GPIO2.

## Status
Rev 2 establishes the RTC GPIO7/DIO1 deep-sleep wake architecture. Production
validation still requires confirming the exact RadioLib API/version and Wio-SX1262
RF-switch control on the assembled hardware. BLE commissioning and authenticated
encryption remain later-stage modules.
