#include "mqttclient.hpp"
#include <mosquitto.h>
#include <iostream>
#include <boost/format.hpp>
#include <boost/noncopyable.hpp>

using std::cout;
using std::cerr;
using boost::format;

class MosquittoLibrarySingleton : boost::noncopyable
{
public:
  // TODO: for the moment keep it dumb and simple.
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

class MQTTClient::MQTTClientImpl //: public boost::enable_shared_from_this
{
public:
  MQTTClientImpl(MQTTClient& owner) : mosq_(NULL), owner_(owner) {}
  bool Init(const char *client_id) {
    // NOTE: need to ensure all callbacks finish before destruction
    // NOTE: NULL returned only on out of memory or invalid argument.
    // NOTE: in either case there is not much that we can do so just run dead.
    mosq_ = mosquitto_new(client_id, true, this);
    if (!mosq_) { return false; }
    mosquitto_connect_callback_set(mosq_, &on_connect);
    mosquitto_message_callback_set(mosq_, &on_message);
    return true;
  }
  ~MQTTClientImpl()
  {
    if (mosq_) { mosquitto_destroy(mosq_); }
  }
  struct mosquitto *mosq_;
private:
  MQTTClient& owner_;
  static void on_connect(struct mosquitto *mosq, void *obj, int rc)
  {
    switch (rc) {
      case 0: break;
      case 1: cerr << "Connection error: procotol version\n"; break;
      case 2: cerr << "Connection error: identifier rejected\n"; break;
      case 3: cerr << "Connection error: broker unavailable\n"; break;
      default: cerr << "Connection error: impossible error\n"; break;
    }
    //(MQTTClientImpl*)obj
  }

  static void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *) { }
};

MQTTClient::MQTTClient(const char *client_id)
: client_id_(client_id),
  impl_(new MQTTClientImpl(*this))
{
  if (!impl_->Init(client_id)) return;
}

MQTTClient::~MQTTClient()
{
}

bool MQTTClient::Connect()
{
  //if (!impl_) return false;
  return true;
}

bool MQTTClient::Subscribe(const char *topic)
{
  // if (!impl_) return false;
  return true;
}

bool MQTTClient::Unsubscribe(const char *topic)
{
  //if (!impl_) return false;
  return true;
}
