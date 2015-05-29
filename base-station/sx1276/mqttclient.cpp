#define _XOPEN_SOURCE 600
#include "mqttclient.hpp"
#include <mosquitto.h>
#include <iostream>
#include <boost/format.hpp>
#include <boost/noncopyable.hpp>
#include <string.h>
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/system_clocks.hpp>

using std::string;
using std::cout;
using std::cerr;
using boost::format;
using boost::chrono::steady_clock;

string safe_perror(const char *txt)
{
  char buf[64];
  strerror_r(errno, buf, sizeof(buf));
  return (txt ? (string(txt) + ": ") : string()) + string(buf);
}

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

MQTTClient::MQTTClient(const char *client_id, const char *host, int port)
  : valid_(false),
    client_id_(client_id),
    broker_host_(host), broker_port_(port), keep_alive_s_(60)
{
}

MQTTClient::~MQTTClient()
{
}

MoqsuittoMQTTClient::MoqsuittoMQTTClient(const char *client_id, const char *host, int port)
  : MQTTClient(client_id, host, port),
    mosq_(NULL),
    connack_state_(NONE)
{
  valid_ = Init();
  if (!valid_) { mosquitto_destroy(mosq_); mosq_ = NULL; }
}

MoqsuittoMQTTClient::~MoqsuittoMQTTClient()
{
  if (mosq_) { mosquitto_destroy(mosq_); }
}

bool MoqsuittoMQTTClient::Init()
{
  // NOTE: need to ensure all callbacks finish before destruction
  // NOTE: NULL returned only on out of memory or invalid argument.
  // NOTE: in either case there is not much that we can do so just run dead.
  mosq_ = mosquitto_new(client_id_.c_str(), true, this);
  if (!mosq_) { last_error_ = safe_perror(NULL); return false; }
  mosquitto_connect_callback_set(mosq_, &on_connect);
  mosquitto_message_callback_set(mosq_, &on_message);
  return true;
}

void MoqsuittoMQTTClient::on_connect(struct mosquitto *mosq, void *obj, int rc)
{
  MoqsuittoMQTTClient* self = static_cast<MoqsuittoMQTTClient*>(obj);
  self->OnConnect(mosq, rc);
}

void MoqsuittoMQTTClient::on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
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
    return safe_perror(txt);
  case MOSQ_ERR_NO_CONN:
    return string(txt) + ": Out of memory";
  case MOSQ_ERR_PROTOCOL:
    return string(txt) + ": Protocol error";
  case MOSQ_ERR_NOMEM:
    return string(txt) + ": Not connected to roker";
  case MOSQ_ERR_INVAL:
    return string(txt) + ": Invalid argument";
  }
  return "Unknown Error";
}

bool MoqsuittoMQTTClient::Connect()
{
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
  if (!valid_) return false;
  last_error_ = "";
  int rc = mosquitto_subscribe(mosq_, NULL, topic, 0);
  last_error_ = MosqErrStr(rc, "Subscribe");
  return rc == MOSQ_ERR_SUCCESS;
}

bool MoqsuittoMQTTClient::Unsubscribe(const char *topic)
{
  if (!valid_) return false;
  last_error_ = "";
  int rc = mosquitto_unsubscribe(mosq_, NULL, topic);
  last_error_ = MosqErrStr(rc, "Unsubscribe");
  return rc == MOSQ_ERR_SUCCESS;
}
