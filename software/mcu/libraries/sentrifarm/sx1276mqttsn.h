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

#ifndef SX1276MQTSN_H__
#define SX1276MQTSN_H__

#include "Arduino.h"
#include <SPI.h>
#include <elapsedMillis.h>
#include "sx1276.h"
#include "mqttsn.h"
#include "mqttsn-messages.h"

class Stream;

// To get things moving in a hurry, I forked arduino-mqtt-sn
// from https://bitbucket.org/MerseyViking/mqtt-sn-arduino
// Downside there is some slightly dodgy callback stuff going on
class MQTTSX1276 : public MQTTSN
{
public:
  MQTTSX1276(SX1276Radio& radio);
  virtual ~MQTTSX1276();

  static byte xorvbuf(const byte* buf, byte len);

  bool IsMaybeConnected() const { return connack_possible_; }

  bool Begin(Stream* DEBUGV);
  bool TryReceive(bool &crc);
  void ResetDisconnect() { got_disconnect_ = 0; }
  bool DidDisconnect() const { return got_disconnect_ > 0; }

protected:
  virtual bool parse_impl(uint8_t* response);
  virtual void send_message_impl(const uint8_t* msg, uint8_t length);
  virtual void willmsgreq_handler(const message_header* msg);
  virtual void connack_handler(const msg_connack* msg);
  virtual void disconnect_handler(const msg_disconnect* msg);
#if 0
  virtual void advertise_handler(const msg_advertise* msg);
  virtual void gwinfo_handler(const msg_gwinfo* msg);
  virtual void connack_handler(const msg_connack* msg);
  virtual void willtopicreq_handler(const message_header* msg);
  virtual void regack_handler(const msg_regack* msg);
  virtual void publish_handler(const msg_publish* msg);
  virtual void register_handler(const msg_register* msg);
  virtual void puback_handler(const msg_puback* msg);
  virtual void suback_handler(const msg_suback* msg);
  virtual void unsuback_handler(const msg_unsuback* msg);
  virtual void pingreq_handler(const msg_pingreq* msg);
  virtual void pingresp_handler();
  virtual void disconnect_handler(const msg_disconnect* msg);
  virtual void willtopicresp_handler(const msg_willtopicresp* msg);
  virtual void willmsgresp_handler(const msg_willmsgresp* msg);
#endif

private:
  SX1276Radio& radio_;
  byte rx_buffer_[128];
  byte rx_buffer_len_;
  byte tx_buffer_[MAX_BUFFER_SIZE+4];
  byte tx_rolling_;
  byte got_disconnect_;

  bool connack_possible_;
};

#endif // SX1276MQTSN_H__
