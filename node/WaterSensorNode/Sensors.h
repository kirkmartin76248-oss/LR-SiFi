#pragma once
#include "Protocol.h"

void initSensors();
void initBatteryADC();
void sensorsPower(bool on);
void readTelemetry(TelemetryPacket& p);
