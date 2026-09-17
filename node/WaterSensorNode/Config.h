#pragma once
#include <Arduino.h>

// XIAO ESP32-C6 pin assignment — Rev 2
#define PIN_TEMP_ADC       0   // D0 / GPIO0
#define PIN_DO_ADC         1   // D1 / GPIO1
#define PIN_BAT_ADC        2   // D2 / GPIO2

#define PIN_RADIO_NSS      21  // D3 / GPIO21
#define PIN_RADIO_DIO1      7  // MTDO / GPIO7 — RTC/deep-sleep wake
#define PIN_RADIO_RF_SW    23  // D5 / GPIO23
#define PIN_RADIO_RESET    16  // D6 / GPIO16
#define PIN_RADIO_BUSY     17  // D7 / GPIO17
#define PIN_RADIO_SCK      19  // D8 / GPIO19
#define PIN_RADIO_MISO     20  // D9 / GPIO20
#define PIN_RADIO_MOSI     18  // D10 / GPIO18

#define PIN_SENSOR_ENABLE   4  // MTMS / GPIO4
#define PIN_WATER_SWITCH    5  // MTDI / GPIO5
#define PIN_AIR_SWITCH      6  // MTCK / GPIO6
#define PIN_SPARE           22  // D4 / GPIO22

#define PIN_LED             15
#define LED_ON              LOW
#define LED_OFF             HIGH

#define DEFAULT_NETWORK_ID  1
#define DEFAULT_HUB_ID      1
#define DEFAULT_NODE_ID     1

#define DEFAULT_INTERVAL_SEC 3600UL
#define MIN_INTERVAL_SEC     300UL
#define MAX_INTERVAL_SEC     43200UL

#define DEFAULT_FREQUENCY_HZ 915000000UL
#define DEFAULT_SF           7
#define DEFAULT_BW_HZ        125000UL
#define DEFAULT_CR           5
#define DEFAULT_TX_POWER_DBM 14

#define SENSOR_SETTLE_MS     2000UL
#define ACK_TIMEOUT_MS       500UL

#define RADIO_RX_MS          20UL
#define RADIO_SLEEP_MS       1000UL
#define HUB_PREAMBLE_SYMBOLS 1465

#define BAT_DIVIDER_RATIO    2.0f
#define ADC_FULL_SCALE_MV    3300.0f
#define ADC_MAX_COUNTS       4095.0f
