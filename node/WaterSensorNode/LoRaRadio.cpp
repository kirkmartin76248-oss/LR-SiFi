#include "LoRaRadio.h"
#include "Config.h"

static volatile bool radioIRQ = false;
void IRAM_ATTR LoRaRadio::dio1ISR() { radioIRQ = true; }

LoRaRadio::LoRaRadio()
  : radio(new Module(PIN_RADIO_NSS, PIN_RADIO_DIO1, PIN_RADIO_RESET, PIN_RADIO_BUSY, SPI)) {}

bool LoRaRadio::begin(const NodeConfig& cfg) {
  activeCfg = cfg;
  SPI.begin(PIN_RADIO_SCK, PIN_RADIO_MISO, PIN_RADIO_MOSI, PIN_RADIO_NSS);
  int16_t state = radio.begin(cfg.frequencyHz/1000000.0, cfg.bandwidthHz/1000.0, cfg.spreadingFactor, cfg.codingRate, 0x12, cfg.txPowerDbm, 8, 0);
  if (state != RADIOLIB_ERR_NONE) return false;
  radio.setDio1Action(dio1ISR);
  return true;
}

void LoRaRadio::applyConfig(const NodeConfig& cfg) {
  activeCfg = cfg;
  radio.setFrequency(cfg.frequencyHz/1000000.0);
  radio.setSpreadingFactor(cfg.spreadingFactor);
  radio.setBandwidth(cfg.bandwidthHz/1000.0);
  radio.setCodingRate(cfg.codingRate);
  radio.setOutputPower(cfg.txPowerDbm);
}

bool LoRaRadio::startDutyCycleRX() {
  radioIRQ = false;
  return radio.startReceiveDutyCycle(RADIO_RX_MS, RADIO_SLEEP_MS, RADIOLIB_IRQ_RX_DEFAULT_FLAGS, RADIOLIB_IRQ_RX_DEFAULT_MASK) == RADIOLIB_ERR_NONE;
}

bool LoRaRadio::readPacket(uint8_t* buf, size_t len, uint32_t timeoutMs) {
  uint32_t start = millis();
  while (!radioIRQ && millis()-start < timeoutMs) delay(1);
  if (!radioIRQ) return false;
  radioIRQ = false;
  if (radio.getPacketLength() != len) { radio.clearIrqFlags(); return false; }
  return radio.readData(buf, len) == RADIOLIB_ERR_NONE;
}

bool LoRaRadio::receivePoll(PacketHeader& hdr, PollPacket& poll) {
  uint8_t buf[sizeof(PollPacket)];
  if (!readPacket(buf,sizeof(buf),300)) return false;
  memcpy(&poll,buf,sizeof(poll)); hdr=poll.h; return true;
}

bool LoRaRadio::sendTelemetry(const TelemetryPacket& packet) {
  return radio.transmit(reinterpret_cast<const uint8_t*>(&packet),sizeof(packet)) == RADIOLIB_ERR_NONE;
}

bool LoRaRadio::receiveAck(AckPacket& packet,uint32_t timeoutMs) {
  radioIRQ=false;
  if (radio.startReceive()!=RADIOLIB_ERR_NONE) return false;
  if (!readPacket(reinterpret_cast<uint8_t*>(&packet),sizeof(packet),timeoutMs)) { radio.standby(); return false; }
  radio.standby(); return true;
}
