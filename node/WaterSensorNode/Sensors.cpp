#include "Sensors.h"
#include "Config.h"

void initSensors() {
  pinMode(PIN_SENSOR_ENABLE, OUTPUT);
  digitalWrite(PIN_SENSOR_ENABLE, LOW);
  pinMode(PIN_WATER_SWITCH, INPUT_PULLUP);
  pinMode(PIN_AIR_SWITCH, INPUT_PULLUP);
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_TEMP_ADC, ADC_11db);
  analogSetPinAttenuation(PIN_DO_ADC, ADC_11db);
  analogSetPinAttenuation(PIN_BAT_ADC, ADC_11db);
}

void initBatteryADC() { pinMode(PIN_BAT_ADC, INPUT); }
void sensorsPower(bool on) { digitalWrite(PIN_SENSOR_ENABLE, on ? HIGH : LOW); }

static uint16_t adcMilliVolts(uint8_t pin) {
  return (uint16_t)analogReadMilliVolts(pin);
}

void readTelemetry(TelemetryPacket& p) {
  p.temperatureMv = adcMilliVolts(PIN_TEMP_ADC);
  p.dissolvedOxygenMv = adcMilliVolts(PIN_DO_ADC);
  uint16_t adcBat = adcMilliVolts(PIN_BAT_ADC);
  p.batteryMv = (uint16_t)(adcBat * BAT_DIVIDER_RATIO + 0.5f);
  p.waterFlow = (digitalRead(PIN_WATER_SWITCH) == LOW) ? 1 : 0;
  p.airFlow = (digitalRead(PIN_AIR_SWITCH) == LOW) ? 1 : 0;
}
