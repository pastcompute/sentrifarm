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
#include <string>
#include <iostream>
#include <boost/format.hpp>
#include "libsocket/inetserverdgram.hpp"
#include "util.hpp"

using std::string;
using std::cout;
using std::cerr;
using boost::format;

int main(int argc, char *argv[])
{
  try {
    libsocket::inet_dgram_server srv("127.0.0.1", "1884", LIBSOCKET_BOTH);
    for (;;) {
      uint8_t buffer[512];
      string from, fromport;
      int n = srv.rcvfrom(buffer, sizeof(buffer), from, fromport);
      if (n > 0) {
        cerr << format("%s:%d : %s\n") % from % fromport % util::buf2str(buffer,n);

        // TODO:
        // 1. Push binary onto fifo or stdout or whatever
        // 2. Concurrently read stdin/fifo or whatever and send back out the UDP socket
      }
    }
    srv.destroy();
  } catch (const libsocket::socket_exception& exc) {
      cerr << exc.mesg;
  }
}

