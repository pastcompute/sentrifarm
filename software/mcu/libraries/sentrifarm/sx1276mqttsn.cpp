/*
  Copyright (c) 2015 Andrew McDonnell <bugs@andrewmcdonnell.net>

  This file is part of SentriFarm Radio Relay.

  SentriFarm Radio Relay is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  SentriFarm Radio Relay is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with SentriFarm Radio Relay.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdint.h>
#include <mqttsn.h>
#include "sx1276mqttsn.h"
#if defined(ESP8266)
#include <ets_sys.h>
#else
#define ICACHE_FLASH_ATTR
#endif

#define VERBOSE 1

#ifdef TEENSYDUINO
#define Serial Serial1
#endif

#if VERBOSE
#include <stdio.h>
#define DEBUG(x ...) { char buf[128]; snprintf(buf, sizeof(buf), x); Serial.print(buf); }
#else
#define DEBUG(x ...)
#endif

ICACHE_FLASH_ATTR
MQTTSX1276::MQTTSX1276(SX1276Radio& radio)
  : radio_(radio), rx_buffer_len_(0), tx_rolling_(0),
    connack_possible_(false)
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
    DEBUG("[RX] %d bytes, crc=%d\n\r", rx_buffer_len_, crc);
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

byte MQTTSX1276::xorvbuf(const byte* buf, byte len)
{
  uint8_t xorv = 0;
  for (unsigned j=0; j < len; j++) {
    xorv = xorv ^ buf[j];
  }
  return xorv;
}


ICACHE_FLASH_ATTR
void MQTTSX1276::send_message_impl(const uint8_t* msg, uint8_t length)
{
  tx_buffer_[0] = 0;
  tx_buffer_[1] = tx_rolling_;
  tx_buffer_[2] = 0; // echo counter
  if (length > MAX_BUFFER_SIZE) {
    DEBUG("TX TRUNCATION!\n\r");
    length = MAX_BUFFER_SIZE;
  }
  memcpy(tx_buffer_+3, msg, length);
  byte xorv = xorvbuf(tx_buffer_, length+3);
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
  willmsg("", 0);
}


ICACHE_FLASH_ATTR
void MQTTSX1276::connack_handler(const msg_connack* msg)
{
  // So we have a conversation going?
  // We only get here when we got a connack after _we_ sent a connect
  // So I have no idea why RSMB sends _us_ a CONNACK!
  // Perhaps it is the broker trying to 'connect' with another broker
  connack_possible_ = true;
}

ICACHE_FLASH_ATTR
void MQTTSX1276::disconnect_handler(const msg_disconnect* msg)
{
  got_disconnect_ ++;
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
void MQTTSX1276::willtopicresp_handler(const msg_willtopicresp* msg)
{
}

ICACHE_FLASH_ATTR
void MQTTSX1276::willmsgresp_handler(const msg_willmsgresp* msg)
{
}
#endif
