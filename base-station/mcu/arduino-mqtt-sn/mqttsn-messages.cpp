/*
mqttsn-messages.cpp

The MIT License (MIT)

Copyright (C) 2014 John Donovan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <Arduino.h>
#if defined(ESP8266)
#include <ets_sys.h>
#else
#define ICACHE_FLASH_ATTR
#endif
#define DEBUG_INL 1
#include "mqttsn-messages.h"
#include "mqttsn.h"

#define VERBOSE 1

#if VERBOSE
#include <stdio.h>
#define DEBUG(x ...) { char buf[128]; snprintf(buf, sizeof(buf), x); Serial.print(buf); }
#else
#define DEBUG(x ...)
#endif

#ifdef USE_RF12
#include <JeeLib.h>
#endif

ICACHE_FLASH_ATTR
MQTTSN::MQTTSN() :
waiting_for_response(true),
response_to_wait_for(ADVERTISE),
_message_id(0),
topic_count(0),
_gateway_id(0),
_response_timer(0),
_response_retries(0)
{
    memset(topic_table, 0, sizeof(topic) * MAX_TOPICS);
    memset(message_buffer, 0, MAX_BUFFER_SIZE);
    memset(response_buffer, 0, MAX_BUFFER_SIZE);
}

ICACHE_FLASH_ATTR
MQTTSN::~MQTTSN() {
}

ICACHE_FLASH_ATTR
bool MQTTSN::wait_for_response() {
    if (waiting_for_response) {
        // TODO: Watch out for overflow.
        if ((millis() - _response_timer) > (T_RETRY * 1000L)) {
            _response_timer = millis();

            if (_response_retries == 0) {
                waiting_for_response = false;
                disconnect_handler(NULL);
            } else {
                send_message();
            }

            --_response_retries;
        }
    }

    return waiting_for_response;
}

uint16_t MQTTSN::bswap(const uint16_t val) {
    return (val << 8) | (val >> 8);
}

ICACHE_FLASH_ATTR
uint16_t MQTTSN::find_topic_id(const char* name, uint8_t& index) {
    for (uint8_t i = 0; i < topic_count; ++i) {
        if (strcmp(topic_table[i].name, name) == 0) {
            index = i;
            return topic_table[i].id;
        }
    }

    return 0xffff;
}

ICACHE_FLASH_ATTR
void MQTTSN::parse() {
  if (parse_impl(response_buffer)) {
    dispatch();
  }
}

ICACHE_FLASH_ATTR
void MQTTSN::dispatch() {
    message_header* response_message = (message_header*)response_buffer;
    bool handled = true;

    if (response_message->type < MAX_MQTTSN_MSG_TYPE) {
      DEBUG("RX %s\n\r", message_names[response_message->type]);
    } else {
      DEBUG("RX >UNKNOWN %02x\n\r", response_message->type);
    }

    switch (response_message->type) {
    case ADVERTISE:
        if (waiting_for_response && response_to_wait_for == ADVERTISE) {
            advertise_handler((msg_advertise*)response_buffer);
        } else {
            handled = false;
        }
        break;

    case GWINFO:
        gwinfo_handler((msg_gwinfo*)response_buffer);
        break;

    case CONNACK:
        if (waiting_for_response && response_to_wait_for == CONNACK) {
            connack_handler((msg_connack*)response_buffer);
        } else {
            handled = false;
        }
        break;

    case WILLTOPICREQ:
        willtopicreq_handler(response_message);
        break;

    case WILLMSGREQ:
        willmsgreq_handler(response_message);
        break;

    case REGISTER:
        register_handler((msg_register*)response_buffer);
        break;

    case REGACK:
        if (waiting_for_response && response_to_wait_for == REGACK) {
            regack_handler((msg_regack*)response_buffer);
        } else {
            handled = false;
        }
        break;

    case PUBLISH:
        publish_handler((msg_publish*)response_buffer);
        break;

    case PUBACK:
        if (waiting_for_response && response_to_wait_for == PUBACK) {
            puback_handler((msg_puback*)response_buffer);
        } else {
            handled = false;
        }
        break;

    case SUBACK:
        if (waiting_for_response && response_to_wait_for == SUBACK) {
            suback_handler((msg_suback*)response_buffer);
        } else {
            handled = false;
        }
        break;

    case UNSUBACK:
        if (waiting_for_response && response_to_wait_for == UNSUBACK) {
            unsuback_handler((msg_unsuback*)response_buffer);
        } else {
            handled = false;
        }
        break;

    case PINGREQ:
        pingreq_handler((msg_pingreq*)response_buffer);
        break;

    case PINGRESP:
        if (waiting_for_response && response_to_wait_for == PINGRESP) {
            pingresp_handler();
        } else {
            handled = false;
        }
        break;

    case DISCONNECT:
        disconnect_handler((msg_disconnect*)response_buffer);
        break;

    case WILLTOPICRESP:
        if (waiting_for_response && response_to_wait_for == WILLTOPICRESP) {
            willtopicresp_handler((msg_willtopicresp*)response_buffer);
        } else {
            handled = false;
        }
        break;

    case WILLMSGRESP:
        if (waiting_for_response && response_to_wait_for == WILLMSGRESP) {
            willmsgresp_handler((msg_willmsgresp*)response_buffer);
        } else {
            handled = false;
        }
        break;

    default:
        return;
    }

    if (handled) {
        waiting_for_response = false;
    }
}

ICACHE_FLASH_ATTR
void MQTTSN::send_message() {
    message_header* hdr = reinterpret_cast<message_header*>(message_buffer);
    DEBUG("TX %s\n\r", hdr->type < MAX_MQTTSN_MSG_TYPE ? message_names[hdr->type] : ">UNKNOWN");
    send_message_impl(message_buffer, hdr->length);
    if (!waiting_for_response) {
        _response_timer = millis();
        _response_retries = N_RETRY;

        // Cheesy hack to ensure two messages don't run-on into one send.
//        delay(10);
    }
}

ICACHE_FLASH_ATTR
void MQTTSN::timeout() {
    waiting_for_response = true;
    response_to_wait_for = ADVERTISE;
}

ICACHE_FLASH_ATTR
void MQTTSN::advertise_handler(const msg_advertise* msg) {
    _gateway_id = msg->gw_id;
}

ICACHE_FLASH_ATTR
void MQTTSN::gwinfo_handler(const msg_gwinfo* msg) {
}

ICACHE_FLASH_ATTR
void MQTTSN::connack_handler(const msg_connack* msg) {
}

ICACHE_FLASH_ATTR
void MQTTSN::willtopicreq_handler(const message_header* msg) {
}

ICACHE_FLASH_ATTR
void MQTTSN::willmsgreq_handler(const message_header* msg) {
}

ICACHE_FLASH_ATTR
void MQTTSN::regack_handler(const msg_regack* msg) {
    if (msg->return_code == 0 && topic_count < MAX_TOPICS && bswap(msg->message_id) == _message_id) {
        const uint16_t topic_id = bswap(msg->topic_id);

        bool found_topic = false;

        for (uint8_t i = 0; i < topic_count; ++i) {
            if (topic_table[i].id == topic_id) {
                found_topic = true;
                break;
            }
        }

        if (!found_topic) {
            topic_table[topic_count].id = topic_id;
            ++topic_count;
        }
    }
}

ICACHE_FLASH_ATTR
void MQTTSN::puback_handler(const msg_puback* msg) {
}

#ifdef USE_QOS2
void MQTTSN::pubrec_handler(const msg_pubqos2* msg) {
}

void MQTTSN::pubrel_handler(const msg_pubqos2* msg) {
}

void MQTTSN::pubcomp_handler(const msg_pubqos2* msg) {
}
#endif

ICACHE_FLASH_ATTR
void MQTTSN::pingreq_handler(const msg_pingreq* msg) {
    pingresp();
}

ICACHE_FLASH_ATTR
void MQTTSN::suback_handler(const msg_suback* msg) {
}

ICACHE_FLASH_ATTR
void MQTTSN::unsuback_handler(const msg_unsuback* msg) {
}

ICACHE_FLASH_ATTR
void MQTTSN::disconnect_handler(const msg_disconnect* msg) {
}

ICACHE_FLASH_ATTR
void MQTTSN::pingresp_handler() {
}

ICACHE_FLASH_ATTR
void MQTTSN::publish_handler(const msg_publish* msg) {
    if (msg->flags & FLAG_QOS_1) {
        return_code_t ret = REJECTED_INVALID_TOPIC_ID;
        const uint16_t topic_id = bswap(msg->topic_id);

        for (uint8_t i = 0; i < topic_count; ++i) {
            if (topic_table[i].id == topic_id) {
                ret = ACCEPTED;
                break;
            }
        }

        puback(msg->topic_id, msg->message_id, ret);
    }
}

ICACHE_FLASH_ATTR
void MQTTSN::register_handler(const msg_register* msg) {
    return_code_t ret = REJECTED_INVALID_TOPIC_ID;
    uint8_t index;
    find_topic_id(msg->topic_name, index);

    if (index != 0xffff) {
        topic_table[index].id = bswap(msg->topic_id);
        ret = ACCEPTED;
    }

    regack(msg->topic_id, msg->message_id, ret);
}

ICACHE_FLASH_ATTR
void MQTTSN::willtopicresp_handler(const msg_willtopicresp* msg) {
}

ICACHE_FLASH_ATTR
void MQTTSN::willmsgresp_handler(const msg_willmsgresp* msg) {
}

ICACHE_FLASH_ATTR
void MQTTSN::searchgw(const uint8_t radius) {
    msg_searchgw* msg = reinterpret_cast<msg_searchgw*>(message_buffer);

    msg->length = sizeof(msg_searchgw);
    msg->type = SEARCHGW;
    msg->radius = radius;

    send_message();
    waiting_for_response = true;
    response_to_wait_for = GWINFO;
}

ICACHE_FLASH_ATTR
void MQTTSN::connect(const uint8_t flags, const uint16_t duration, const char* client_id) {
    msg_connect* msg = reinterpret_cast<msg_connect*>(message_buffer);

    msg->length = sizeof(msg_connect) + strlen(client_id);
    msg->type = CONNECT;
    msg->flags = flags;
    msg->protocol_id = PROTOCOL_ID;
    msg->duration = bswap(duration);
    strcpy(msg->client_id, client_id);

    send_message();
    waiting_for_response = true;
    response_to_wait_for = CONNACK;
}

ICACHE_FLASH_ATTR
void MQTTSN::willtopic(const uint8_t flags, const char* will_topic, const bool update) {
    if (will_topic == NULL) {
        message_header* msg = reinterpret_cast<message_header*>(message_buffer);

        msg->type = update ? WILLTOPICUPD : WILLTOPIC;
        msg->length = sizeof(message_header);
    } else {
        msg_willtopic* msg = reinterpret_cast<msg_willtopic*>(message_buffer);

        msg->type = update ? WILLTOPICUPD : WILLTOPIC;
        msg->flags = flags;
        strcpy(msg->will_topic, will_topic);
    }

    send_message();

//    if ((flags & QOS_MASK) == FLAG_QOS_1 || (flags & QOS_MASK) == FLAG_QOS_2) {
//        waiting_for_response = true;
//        response_to_wait_for = WILLMSGREQ;
//    }
}

ICACHE_FLASH_ATTR
void MQTTSN::willmsg(const void* will_msg, const uint8_t will_msg_len, const bool update) {
    msg_willmsg* msg = reinterpret_cast<msg_willmsg*>(message_buffer);

    msg->length = sizeof(msg_willmsg) + will_msg_len;
    msg->type = update ? WILLMSGUPD : WILLMSG;
    memcpy(msg->willmsg, will_msg, will_msg_len);

    send_message();
}

ICACHE_FLASH_ATTR
void MQTTSN::disconnect(const uint16_t duration) {
    msg_disconnect* msg = reinterpret_cast<msg_disconnect*>(message_buffer);

    msg->length = sizeof(message_header);
    msg->type = DISCONNECT;

    if (duration > 0) {
        msg->length += sizeof(msg_disconnect);
        msg->duration = bswap(duration);
    }

    send_message();
    waiting_for_response = true;
    response_to_wait_for = DISCONNECT;
}

ICACHE_FLASH_ATTR
bool MQTTSN::register_topic(const char* name) {
    if (!waiting_for_response && topic_count < (MAX_TOPICS - 1)) {
        ++_message_id;

        // Fill in the next table entry, but we only increment the counter to
        // the next topic when we get a REGACK from the broker. So don't issue
        // another REGISTER until we have resolved this one.
        topic_table[topic_count].name = name;
        topic_table[topic_count].id = 0;

        msg_register* msg = reinterpret_cast<msg_register*>(message_buffer);

        msg->length = sizeof(msg_register) + strlen(name);
        msg->type = REGISTER;
        msg->topic_id = 0;
        msg->message_id = bswap(_message_id);
        strcpy(msg->topic_name, name);

        send_message();
        waiting_for_response = true;
        response_to_wait_for = REGACK;
        return true;
    }

    return false;
}

ICACHE_FLASH_ATTR
void MQTTSN::regack(const uint16_t topic_id, const uint16_t message_id, const return_code_t return_code) {
    msg_regack* msg = reinterpret_cast<msg_regack*>(message_buffer);

    msg->length = sizeof(msg_regack);
    msg->type = REGACK;
    msg->topic_id = bswap(topic_id);
    msg->message_id = bswap(message_id);
    msg->return_code = return_code;

    send_message();
}

ICACHE_FLASH_ATTR
void MQTTSN::publish(const uint8_t flags, const uint16_t topic_id, const void* data, const uint8_t data_len) {
    ++_message_id;

    msg_publish* msg = reinterpret_cast<msg_publish*>(message_buffer);

    msg->length = sizeof(msg_publish) + data_len;
    msg->type = PUBLISH;
    msg->flags = flags;
    msg->topic_id = bswap(topic_id);
    msg->message_id = bswap(_message_id);
    memcpy(msg->data, data, data_len);

    send_message();

    if ((flags & QOS_MASK) == FLAG_QOS_1 || (flags & QOS_MASK) == FLAG_QOS_2) {
        waiting_for_response = true;
        response_to_wait_for = PUBACK;
    }
}

#ifdef USE_QOS2
void MQTTSN::pubrec() {
    msg_pubqos2* msg = reinterpret_cast<msg_pubqos2*>(message_buffer);
    msg->length = sizeof(msg_pubqos2);
    msg->type = PUBREC;
    msg->message_id = bswap(_message_id);

    send_message();
}

void MQTTSN::pubrel() {
    msg_pubqos2* msg = reinterpret_cast<msg_pubqos2*>(message_buffer);
    msg->length = sizeof(msg_pubqos2);
    msg->type = PUBREL;
    msg->message_id = bswap(_message_id);

    send_message();
}

void MQTTSN::pubcomp() {
    msg_pubqos2* msg = reinterpret_cast<msg_pubqos2*>(message_buffer);
    msg->length = sizeof(msg_pubqos2);
    msg->type = PUBCOMP;
    msg->message_id = bswap(_message_id);

    send_message();
}
#endif

ICACHE_FLASH_ATTR
void MQTTSN::puback(const uint16_t topic_id, const uint16_t message_id, const return_code_t return_code) {
    msg_puback* msg = reinterpret_cast<msg_puback*>(message_buffer);

    msg->length = sizeof(msg_puback);
    msg->type = PUBACK;
    msg->topic_id = bswap(topic_id);
    msg->message_id = bswap(message_id);
    msg->return_code = return_code;

    send_message();
}

ICACHE_FLASH_ATTR
void MQTTSN::subscribe_by_name(const uint8_t flags, const char* topic_name) {
    ++_message_id;

    msg_subscribe* msg = reinterpret_cast<msg_subscribe*>(message_buffer);

    // The -2 here is because we're unioning a 0-length member (topic_name)
    // with a uint16_t in the msg_subscribe struct.
    msg->length = sizeof(msg_subscribe) + strlen(topic_name) - 2;
    msg->type = SUBSCRIBE;
    msg->flags = (flags & QOS_MASK) | FLAG_TOPIC_NAME;
    msg->message_id = bswap(_message_id);
    strcpy(msg->topic_name, topic_name);

    send_message();

    if ((flags & QOS_MASK) == FLAG_QOS_1 || (flags & QOS_MASK) == FLAG_QOS_2) {
        waiting_for_response = true;
        response_to_wait_for = SUBACK;
    }
}

ICACHE_FLASH_ATTR
void MQTTSN::subscribe_by_id(const uint8_t flags, const uint16_t topic_id) {
    ++_message_id;

    msg_subscribe* msg = reinterpret_cast<msg_subscribe*>(message_buffer);

    msg->length = sizeof(msg_subscribe);
    msg->type = SUBSCRIBE;
    msg->flags = (flags & QOS_MASK) | FLAG_TOPIC_PREDEFINED_ID;
    msg->message_id = bswap(_message_id);
    msg->topic_id = bswap(topic_id);

    send_message();

    if ((flags & QOS_MASK) == FLAG_QOS_1 || (flags & QOS_MASK) == FLAG_QOS_2) {
        waiting_for_response = true;
        response_to_wait_for = SUBACK;
    }
}

ICACHE_FLASH_ATTR
void MQTTSN::unsubscribe_by_name(const uint8_t flags, const char* topic_name) {
    ++_message_id;

    msg_unsubscribe* msg = reinterpret_cast<msg_unsubscribe*>(message_buffer);

    // The -2 here is because we're unioning a 0-length member (topic_name)
    // with a uint16_t in the msg_unsubscribe struct.
    msg->length = sizeof(msg_unsubscribe) + strlen(topic_name) - 2;
    msg->type = UNSUBSCRIBE;
    msg->flags = (flags & QOS_MASK) | FLAG_TOPIC_NAME;
    msg->message_id = bswap(_message_id);
    strcpy(msg->topic_name, topic_name);

    send_message();

    if ((flags & QOS_MASK) == FLAG_QOS_1 || (flags & QOS_MASK) == FLAG_QOS_2) {
        waiting_for_response = true;
        response_to_wait_for = UNSUBACK;
    }
}

ICACHE_FLASH_ATTR
void MQTTSN::unsubscribe_by_id(const uint8_t flags, const uint16_t topic_id) {
    ++_message_id;

    msg_unsubscribe* msg = reinterpret_cast<msg_unsubscribe*>(message_buffer);

    msg->length = sizeof(msg_unsubscribe);
    msg->type = UNSUBSCRIBE;
    msg->flags = (flags & QOS_MASK) | FLAG_TOPIC_PREDEFINED_ID;
    msg->message_id = bswap(_message_id);
    msg->topic_id = bswap(topic_id);

    send_message();

    if ((flags & QOS_MASK) == FLAG_QOS_1 || (flags & QOS_MASK) == FLAG_QOS_2) {
        waiting_for_response = true;
        response_to_wait_for = UNSUBACK;
    }
}

ICACHE_FLASH_ATTR
void MQTTSN::pingreq(const char* client_id) {
    msg_pingreq* msg = reinterpret_cast<msg_pingreq*>(message_buffer);
    msg->length = sizeof(msg_pingreq) + strlen(client_id);
    msg->type = PINGREQ;
    strcpy(msg->client_id, client_id);

    send_message();

    waiting_for_response = true;
    response_to_wait_for = PINGRESP;
}

ICACHE_FLASH_ATTR
void MQTTSN::pingresp() {
    message_header* msg = reinterpret_cast<message_header*>(message_buffer);
    msg->length = sizeof(message_header);
    msg->type = PINGRESP;

    send_message();
}

#ifdef USE_RF12
bool MQTTSNRF12::parse_impl(uint8_t* response) {
    memcpy(response, (const void*)rf12_data, RF12_MAXDATA < MAX_BUFFER_SIZE ? RF12_MAXDATA : MAX_BUFFER_SIZE);
    return true;
}

void MQTTSNRF12::send_message_impl(const uint8_t* msg, uint8_t length)
{
    while (!rf12_canSend()) {
        rf12_recvDone();
        Sleepy::loseSomeTime(32);
    }
    rf12_sendStart(_gateway_id, msg, length);
    rf12_sendWait(2);
}
#endif

ICACHE_FLASH_ATTR
bool MQTTSNSerial::parse_impl(uint8_t* response) {
    if (Serial.available() > 0) {
        uint8_t packet_length = (uint8_t)Serial.read();
        *response++ = packet_length--;
        int n = 1;
        while (packet_length > 0 && n < MAX_BUFFER_SIZE) {
            while (Serial.available() > 0 && n < MAX_BUFFER_SIZE) {
                *response++ = (uint8_t)Serial.read();
                --packet_length;
                n++;
            }
        }
        return true;
    }
    return false;
}

ICACHE_FLASH_ATTR
void MQTTSNSerial::send_message_impl(const uint8_t* msg, uint8_t length)
{
    Serial.write(msg, (size_t)length);
    Serial.flush();
}

