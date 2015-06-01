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
#include "mqttclient.hpp"
#include "util.hpp"
#include <mosquitto.h>
#include <iostream>
#include <boost/format.hpp>
#include <boost/noncopyable.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/system_clocks.hpp>

using std::string;
using std::cout;
using std::cerr;
using boost::format;
using boost::chrono::steady_clock;

#if 0
#define DEBUG(x ...) printf("[DBG] " x)
#define REPORT_DEBUG
#else
#define DEBUG(x ...)
#endif

class MosquittoLibrarySingleton : boost::noncopyable
{
public:
  // For the moment keep it dumb and simple : create one instance as a module private static.
  MosquittoLibrarySingleton() {
    mosquitto_lib_init();
  }
  ~MosquittoLibrarySingleton() {
    mosquitto_lib_cleanup();
  }
  void ReportVersion() {
    int v1, v2, vr;
    mosquitto_lib_version(&v1, &v2, &vr);
    cout << format("Mosquitto library version: %d.%dr%d\n") % v1 % v2 % vr;
  }
};

static MosquittoLibrarySingleton mosquitto;

/// Implementation shim wrapping mosquitto C library.
/// For the time being keep things simple and use the loop / non threaded version.
class MoqsuittoMQTTClient : public MQTTClient
{
public:
  MoqsuittoMQTTClient(const char *client_id, const char *host, int port);
  virtual ~MoqsuittoMQTTClient();
  virtual bool have_connack() const { return connack_state_ == HAVE; }
  virtual bool Connect();
  virtual bool Poll();
  virtual bool Subscribe(const char *topic);
  virtual bool Unsubscribe(const char *topic);

private:
  bool Init();
  void OnConnect(struct mosquitto *mosq, int rc);
  void OnMessage(struct mosquitto *mosq, const struct mosquitto_message *msg);

  struct mosquitto *mosq_;

  static void on_connect(struct mosquitto *mosq, void *obj, int rc);
  static void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *);

  enum ConnackState { NONE, WANT, HAVE, ERROR };
  ConnackState connack_state_;
};

boost::shared_ptr<MQTTClient> MQTTClient::CreateInstance(const char *client_id, const char *host, int port)
{
  if (host == NULL) { host = "127.0.0.1"; }
  return boost::shared_ptr<MQTTClient>(new MoqsuittoMQTTClient(client_id, host, port));
}

MoqsuittoMQTTClient::MoqsuittoMQTTClient(const char *client_id, const char *host, int port)
  : MQTTClient(client_id, host, port),
    mosq_(NULL),
    connack_state_(NONE)
{
#ifdef REPORT_DEBUG
  mosquitto.ReportVersion();
#endif
  valid_ = Init();
  if (!valid_) { mosquitto_destroy(mosq_); mosq_ = NULL; }
}

MoqsuittoMQTTClient::~MoqsuittoMQTTClient()
{
  DEBUG("Destructor()\n");
  if (mosq_) { mosquitto_destroy(mosq_); }
}

bool MoqsuittoMQTTClient::Init()
{
  DEBUG("Init() %s %s:%d\n", client_id_.c_str(), broker_host_.c_str(), broker_port_);
  // NOTE: need to ensure all callbacks finish before destruction
  // NOTE: NULL returned only on out of memory or invalid argument.
  // NOTE: in either case there is not much that we can do so just run dead.
  mosq_ = mosquitto_new(client_id_.c_str(), true, this);
  if (!mosq_) { last_error_ = util::safe_perror(errno, NULL); return false; }
  mosquitto_connect_callback_set(mosq_, &on_connect);
  mosquitto_message_callback_set(mosq_, &on_message);
  return true;
}

void MoqsuittoMQTTClient::on_connect(struct mosquitto *mosq, void *obj, int rc)
{
  DEBUG("on_connect()\n");
  MoqsuittoMQTTClient* self = static_cast<MoqsuittoMQTTClient*>(obj);
  self->OnConnect(mosq, rc);
}

void MoqsuittoMQTTClient::on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
  DEBUG("on_message()\n");
  MoqsuittoMQTTClient* self = static_cast<MoqsuittoMQTTClient*>(obj);
  self->OnMessage(mosq, msg);
}

void MoqsuittoMQTTClient::OnConnect(struct mosquitto *mosq, int rc)
{
  assert(mosq == mosq_);
  switch (rc) {
    case 0:
      if (connack_state_ == WANT) { connack_state_ = HAVE; if (connack_fcn_) { connack_fcn_(client_id_.c_str(), false); } return; }
      std::cerr << "Spurious CONNACK!\n";
      return;
    case 1: last_error_ = "Connection error: procotol version\n"; break;
    case 2: last_error_ = "Connection error: identifier rejected\n"; break;
    case 3: last_error_ = "Connection error: broker unavailable\n"; break;
    default: assert(0);
  }
  connack_state_ = ERROR;
  if (connack_fcn_) { connack_fcn_(client_id_.c_str(), true); }
}

void MoqsuittoMQTTClient::OnMessage(struct mosquitto *mosq, const struct mosquitto_message *msg)
{
  assert(mosq == mosq_);
  if (!message_fcn_) return;
  message_fcn_(client_id_.c_str(), msg->topic, msg->payload, msg->payloadlen);
}

std::string MosqErrStr(int code, const char *txt)
{
  switch (code) {
  case MOSQ_ERR_SUCCESS:
    return "ok";
  case MOSQ_ERR_ERRNO:
    return util::safe_perror(errno, txt);
  case MOSQ_ERR_NO_CONN:
    return string(txt) + ": Out of memory";
  case MOSQ_ERR_PROTOCOL:
    return string(txt) + ": Protocol error";
  case MOSQ_ERR_NOMEM:
    return string(txt) + ": Not connected to broker";
  case MOSQ_ERR_INVAL:
    return string(txt) + ": Invalid argument";
  }
  return "Unknown Error";
}

bool MoqsuittoMQTTClient::Connect()
{
  DEBUG("Connect()\n");
  if (!valid_) return false;
  last_error_ = "";
  // This function is not re-entrant
  connack_state_ = WANT;
  int rc = mosquitto_connect(mosq_, broker_host_.c_str(), broker_port_, keep_alive_s_);
  last_error_ = MosqErrStr(rc, "Unable to connect to broker");
  return rc == MOSQ_ERR_SUCCESS;
}

bool MoqsuittoMQTTClient::Poll()
{
  DEBUG("Poll()\n");
  if (!valid_) return false;
  last_error_ = "";
  int rc = mosquitto_loop(mosq_, 1000, 1);
  if (rc == MOSQ_ERR_CONN_LOST) {
    connack_state_ = WANT;
    rc = mosquitto_reconnect(mosq_);
  }
  last_error_ = MosqErrStr(rc, "Broker poll error");
  return rc == MOSQ_ERR_SUCCESS;
}

bool MoqsuittoMQTTClient::Subscribe(const char *topic)
{
  DEBUG("Subscribe(%s)\n", topic);
  if (!valid_) return false;
  last_error_ = "";
  int rc = mosquitto_subscribe(mosq_, NULL, topic, 0);
  last_error_ = MosqErrStr(rc, "Subscribe");
  return rc == MOSQ_ERR_SUCCESS;
}

bool MoqsuittoMQTTClient::Unsubscribe(const char *topic)
{
  DEBUG("Unsubscribe(%s)\n", topic);
  if (!valid_) return false;
  last_error_ = "";
  int rc = mosquitto_unsubscribe(mosq_, NULL, topic);
  last_error_ = MosqErrStr(rc, "Unsubscribe");
  return rc == MOSQ_ERR_SUCCESS;
}
