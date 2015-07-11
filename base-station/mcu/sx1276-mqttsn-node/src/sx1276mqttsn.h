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

  bool Begin(Stream* DEBUGV);
  bool TryReceive(bool &crc);

protected:
  virtual bool parse_impl(uint8_t* response);
  virtual void send_message_impl(const uint8_t* msg, uint8_t length);
  virtual void willmsgreq_handler(const message_header* msg);
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
};

#endif // SX1276MQTSN_H__
