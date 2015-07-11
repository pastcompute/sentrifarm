#include "sx1276mqttsn.h"
#if defined(ESP8266)
#include <ets_sys.h>
#else
#define ICACHE_FLASH_ATTR
#endif

#define VERBOSE 1

#if VERBOSE
#include <stdio.h>
#define DEBUG(x ...) { char buf[128]; snprintf(buf, sizeof(buf), x); Serial.print(buf); }
#else
#define DEBUG(x ...)
#endif

ICACHE_FLASH_ATTR
MQTTSX1276::MQTTSX1276(SX1276Radio& radio)
  : radio_(radio), rx_buffer_len_(0), tx_rolling_(0)
{
}

ICACHE_FLASH_ATTR
MQTTSX1276::~MQTTSX1276()
{
}

ICACHE_FLASH_ATTR
bool MQTTSX1276::Begin(Stream* debug)
{
  bool started_ok  = false;
  // init SPI and then program the chip to LoRa mode
  SPI.begin();
  if (debug) { debug->print(F("SX1276: version=")); debug->println(radio_.ReadVersion()); }
  if (!radio_.Begin()) {
    if (debug) { debug->println(F("SX1276 init error")); }
  } else {
    radio_.SetCarrier(919000000);
    uint32_t carrier_hz = 0;
    radio_.ReadCarrier(carrier_hz);
    if (debug) { debug->print(F("Carrier: ")); debug->println(carrier_hz); }
    started_ok = true;
  }
  SPI.end();
  return started_ok;
}

// Kind of dodgy receive and process in polling loop
ICACHE_FLASH_ATTR
bool MQTTSX1276::TryReceive(bool& crc)
{
  crc = false;
  SPI.begin();
  rx_buffer_len_ = 0;
  if (radio_.ReceiveMessage(rx_buffer_, sizeof(rx_buffer_), rx_buffer_len_, crc))
  {
    SPI.end();
    parse(); // <-- calls parse_impl()
    return true;
  }
  SPI.end();
  return false;
}

ICACHE_FLASH_ATTR
bool MQTTSX1276::parse_impl(uint8_t* response)
{
  if (rx_buffer_len_ < 3) { DEBUG("SHORT MESSAGE!\n\r"); return true; }
  // Ooops, our carambola is still using the dogdgy 3-byte header
  DEBUG("RX CTR=%d\n\r", rx_buffer_[1]);

  if (rx_buffer_[0] == 0x2) { DEBUG("RX HELLO\n\r"); return true; }

  // If we got a message > 66 bytes not much we can do about it for now
  memcpy(response, rx_buffer_+3, rx_buffer_len_-3 > MAX_BUFFER_SIZE ? MAX_BUFFER_SIZE : rx_buffer_len_-3);
  return true; // <-- call dispatch()
}

ICACHE_FLASH_ATTR
void MQTTSX1276::send_message_impl(const uint8_t* msg, uint8_t length)
{
  tx_buffer_[0] = 0;
  tx_buffer_[1] = tx_rolling_;
  tx_buffer_[2] = 0; // echo counter
  memcpy(tx_buffer_+3, msg, length);
  uint8_t xorv = 0;
  for (unsigned j=0; j < length+3; j++) {
    xorv = xorv ^ tx_buffer_[j];
  }
  tx_buffer_[length + 3] = xorv;

  DEBUG("TX CNT=%d XOR=%02x payload=%d\n\r", tx_rolling_, xorv, length)
  tx_rolling_ ++;
  SPI.begin();
  radio_.TransmitMessage(tx_buffer_, length + 4);
  radio_.Standby();
  SPI.end();
}

ICACHE_FLASH_ATTR
void MQTTSX1276::willmsgreq_handler(const message_header* msg)
{
  // To keep the RSMB broker jahappy we need to reply to this
  willmsg("", 0);
}

#if 0
ICACHE_FLASH_ATTR
void MQTTSX1276::advertise_handler(const msg_advertise* msg)
{
}

ICACHE_FLASH_ATTR
void MQTTSX1276::gwinfo_handler(const msg_gwinfo* msg)
{
}

ICACHE_FLASH_ATTR
void MQTTSX1276::connack_handler(const msg_connack* msg)
{
}

ICACHE_FLASH_ATTR
void MQTTSX1276::willtopicreq_handler(const message_header* msg)
{
}

ICACHE_FLASH_ATTR
void MQTTSX1276::regack_handler(const msg_regack* msg)
{
}

ICACHE_FLASH_ATTR
void MQTTSX1276::publish_handler(const msg_publish* msg)
{
}

ICACHE_FLASH_ATTR
void MQTTSX1276::register_handler(const msg_register* msg)
{
}

ICACHE_FLASH_ATTR
void MQTTSX1276::puback_handler(const msg_puback* msg)
{
}

ICACHE_FLASH_ATTR
void MQTTSX1276::suback_handler(const msg_suback* msg)
{
}

ICACHE_FLASH_ATTR
void MQTTSX1276::unsuback_handler(const msg_unsuback* msg)
{
}

ICACHE_FLASH_ATTR
void MQTTSX1276::pingreq_handler(const msg_pingreq* msg)
{
}

ICACHE_FLASH_ATTR
void MQTTSX1276::pingresp_handler()
{
}

ICACHE_FLASH_ATTR
void MQTTSX1276::disconnect_handler(const msg_disconnect* msg)
{
}

ICACHE_FLASH_ATTR
void MQTTSX1276::willtopicresp_handler(const msg_willtopicresp* msg)
{
}

ICACHE_FLASH_ATTR
void MQTTSX1276::willmsgresp_handler(const msg_willmsgresp* msg)
{
}
#endif
