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
using util::buf2str;

struct MsgHandler
{
  void TopicHandler(const char*client_id, const char*topic, const void* payload, unsigned len) {
    cout << format("CLIENT=%s TOPIC=%s PAYLOAD=%s\n") % client_id % topic % buf2str(payload, len);
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
