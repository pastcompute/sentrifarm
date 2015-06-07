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
#include "sx1276.hpp"
#include "sx1276_platform.hpp"
#include "misc.hpp"
#include "util.hpp"
#include "libsocket/inetserverdgram.hpp"
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <string.h>
#include <string>
#include <iostream>

using std::string;
using std::cout;
using std::cerr;
using boost::format;
using boost::shared_ptr;
using boost::thread;
using boost::mutex;
using boost::condition_variable;
using boost::unique_lock;

struct Shared
{
  mutable mutex mx;
  string from_ip;
  string from_port;
  Shared() {}
  void Get(string& ip, string& port) const {
    unique_lock<mutex> lock(mx);
    ip = from_ip;
    port = from_port;
  }
  void Update(const string& ip, const string& port) {
    unique_lock<mutex> lock(mx);
    if (from_ip != ip || from_port != port) { cout << format("Port change: %s %s\n") % ip % port; }
    from_ip = ip;
    from_port = port;
  }

  shared_ptr<SX1276Radio> radio;
  mutex radio_mx;
  // This stuff is a hack   :needs to be abstracted into a thread safe radio instance
  bool Transmit(const void* payload, unsigned len) {
    unique_lock<mutex> lock(radio_mx);
    return radio->SendSimpleMessage(payload, len);
  }
  bool TryReceive(const void* payload, unsigned len, unsigned& rx) {

    // Do a blocking receive, but in such a way we can break it out if Transmit needs to do its thing
    // OTOH dont let a high rate of TX starve receiving

    bool crc_error = false;
    bool timeout = false;
    unsigned timeout_ms = 500; // This will need a lot of tuning probably

    unique_lock<mutex> lock(radio_mx);
    int received = 0;
    do {
      int received = len;
      if (!radio->ReceiveSimpleMessage((uint8_t*)payload, received, timeout_ms, timeout, crc_error)) { return false; }
      // allow pending tx...
      lock.unlock();
      usleep(100); // there must be a better way to do this...
      lock.lock();
    } while (timeout || crc_error);
    rx = received;
    return true;
  }
};

class WorkerThread
{
  libsocket::inet_dgram_server& srv_;
  Shared& shared_;
  bool outward_;
  void OutLoop() {
    for (;;) {
      uint8_t buffer[127];
      string from, fromport;
      int n = srv_.rcvfrom(buffer, sizeof(buffer), from, fromport);
      if (n > 0) {
        cerr << format("[Radio TX] %s:%d : %s\n") % from % fromport % util::buf2str(buffer,n);
        { shared_.Update(from, fromport); }

        shared_.Transmit(buffer, n);

      }
    }
  }
  void InLoop() {
    for (;;) {
      uint8_t buffer[256];
      unsigned r=0;

      // TODO : receive

      // This is the hard bit: we need to _asynchronously_ be receiving
      // The SX1276 supports all sorts of nice stuff, like CAD but we havent gotten into that yet
      // For the moment we need to sit here and wait (receive with block)
      // but with a way of falling out to allow transmits to happen...
      bool have_msg = shared_.TryReceive(buffer, 256, r);

      if (have_msg) { // FIXME
        string ip; string port;
        shared_.Get(ip, port);
        cerr << format("[Radio RX] %s:%s %s\n") % ip % port % util::buf2str(buffer,r);
        try {
          srv_.sndto(buffer, r, ip, port);
        } catch (...) {}
      } else {
//        cout << "punt\n";
      }
    }
  }
public:
  // TODO: abstrfact SX1276 RADIo to Radio, etc
  WorkerThread(libsocket::inet_dgram_server& srv, Shared& shared, bool outward)
  : srv_(srv),
    shared_(shared),
    outward_(outward)
  {}
  void Run() {
    try {
      if (outward_) { OutLoop(); } else { InLoop(); }
    } catch (const libsocket::socket_exception& exc) {
      cerr << exc.mesg;
    }
    cout << "Run Finish\n";
  }
};

int main(int argc, char *argv[])
{
  string UDP_BIND_IP = "127.0.0.1";
  int port = 1884;

  if (argc < 3) { fprintf(stderr, "Usage: %s <spidev> <udp-listen-port>", argv[0]); return 1; }

  if ((port = atoi(argv[2]) < 1)) { cerr << "Invalid port.\n"; return 1; }

  libsocket::inet_dgram_server srv(UDP_BIND_IP, argv[2], LIBSOCKET_BOTH);

  shared_ptr<SX1276Platform> platform = SX1276Platform::GetInstance(argv[1]);
  if (!platform) { PR_ERROR("Unable to create platform instance\n"); return 1; }

  shared_ptr<SPI> spi = platform->GetSPI();
  if (!spi) { PR_ERROR("Unable to get SPI instance\n"); return 1; }

  usleep(100);
  shared_ptr<SX1276Radio> radio(new SX1276Radio(spi));
  cout << format("SX1276 Version: %.2x\n") % radio->version();
  platform->ResetSX1276();
  radio->ChangeCarrier(919000000);
  radio->ApplyDefaultLoraConfiguration();
  cout << format("Carrier Frequency: %uHz\n") % radio->carrier();
  if (radio->fault()) { PR_ERROR("Radio Fault\n"); return 1; }

  Shared shared;
  shared.radio = radio;
  WorkerThread outThread(srv, shared, true);
  WorkerThread inThread(srv, shared, false);

  thread outInstance(boost::bind(&WorkerThread::Run, &outThread));
  thread inInstance(boost::bind(&WorkerThread::Run, &inThread));
  outInstance.join();
  inInstance.join();
  cout << "DONE\n";


}

