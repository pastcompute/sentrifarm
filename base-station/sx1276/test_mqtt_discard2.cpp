#include "mqttclient.hpp"
#include <string>
#include <iostream>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

using std::string;
using std::cout;
using std::cerr;
using boost::format;
using boost::shared_ptr;

string safe_payload(const void *data, unsigned len)
{
  string result;
  for (unsigned i=0; i < len; i++) {
    char c = ((const char*)data)[i];
    result += iscntrl(c) ? '.' : c;
  }
  return result;
}

struct MsgHandler
{
  void TopicHandler(const char*client_id, const char*topic, const void* payload, unsigned len) {
    cout << format("CLIENT=%s TOPIC=%s PAYLOAD=%s\n") % client_id % topic % safe_payload(payload, len);
  }
  void ConnackHandler(const char*client_id, bool error) {
    cout << format("CONNACK=%s %s\n") % client_id % (error?"ERROR":"OK");
  }
};

int main(int argc, char *argv[])
{
  shared_ptr<MQTTClient> client = MQTTClient::CreateInstance(argv[0]);
  MsgHandler handler;
  client->RegisterMessageHandler(boost::bind(&MsgHandler::TopicHandler, handler, _1, _2, _3, _4));
  client->RegisterConnackHandler(boost::bind(&MsgHandler::ConnackHandler, handler, _1, _2));

  if (!client->valid()) { cerr << "init: " << client->last_error() << "\n"; return 1; }

  // One shot test: if we cant connect now then die
  if (!client->Connect()) { cerr << "connect: " << client->last_error() << "\n"; return 1; }
  if (!client->Subscribe("#")) { cerr << "subscribe: " << client->last_error() << "\n"; return 1; }
  cout << "Waiting...\n";
  while (true) {
    if (!client->Poll())  { cerr << "poll: " << client->last_error() << "\n"; return 1; }
    usleep(1e6);
  }
}
