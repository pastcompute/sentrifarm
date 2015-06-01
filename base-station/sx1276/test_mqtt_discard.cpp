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
#include "util.hpp"
#include <mosquitto.h>
#include <string>
#include <iostream>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>

using std::string;
using std::cout;
using std::cerr;
using boost::format;
using boost::shared_ptr;
using util::buf2str;

struct UserData {

};

void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
  if (result == 0) {
    cout << format("Connected: OK\n");
  } else {
    switch (result) {
      case 1: cerr << "Connection error: procotol version\n"; break;
      case 2: cerr << "Connection error: identifier rejected\n"; break;
      case 3: cerr << "Connection error: broker unavailable\n"; break;
      default: cerr << "Connection error: impossible error\n"; break;
    }
  }
}

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
  cout << format("TOPIC='%s', PAYLOAD='%s'\n") % message->topic % buf2str(message->payload, message->payloadlen);
}

// Dumb MQTT LoRa 'relay' - subscribes to everything, compresses selected topics, and dump to stdout in binary
int main(int argc, char *argv[])
{
  struct mosquitto *mosq = NULL;
  UserData userData;
  mosquitto_lib_init();

  int v1, v2, vr;
  mosquitto_lib_version(&v1, &v2, &vr);
  cout << format("Mosquitto library version: %d.%dr%d\n") % v1 % v2 % vr;

  // For the moment, dodgy copy&paste from mosquitto client
  string client = (format("mqtt-discard-test-%d") % getpid()).str();
  mosq = mosquitto_new(client.c_str(), true, &userData);  // <-- note, need to ensure all callbacks finished before out of scope
  if (mosq) do {
    mosquitto_connect_callback_set(mosq, connect_callback);
    mosquitto_message_callback_set(mosq, message_callback);
    int rc;
    rc = mosquitto_connect(mosq, "localhost", 1883, 60);
    if (rc == MOSQ_ERR_ERRNO) { perror("Failed to connect to broker"); }
    if (rc == MOSQ_ERR_INVAL) { cerr << "Configuration error on connect\n"; break; }
    rc = mosquitto_subscribe(mosq, NULL, "#", 0);
    if (rc == MOSQ_ERR_INVAL) { cerr << "Configuration error on subscribe\n"; break; }
    if (rc == MOSQ_ERR_NOMEM) { cerr << "Memory error on subscribe\n"; break; }
    // if (rc == MOSQ_ERR_NO_CONN)
    while (true) {
      rc = mosquitto_loop(mosq, -1, 1);
      if (true && rc != 0) {
        sleep(20);
        mosquitto_reconnect(mosq);
      }
    }
    mosquitto_destroy(mosq);
  } while (false); else { // quick and dodgy break out on error and still cleanup, for now
    perror("Failed to create mosquitto client");
  }

  mosquitto_lib_cleanup();
}
