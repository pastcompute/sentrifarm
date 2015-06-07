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
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/system_clocks.hpp>
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
using boost::chrono::steady_clock;

struct Shared
{
  mutable mutex mx;
  string from_ip;
  string from_port;
	bool have_port;
	uint8_t counter;
	uint8_t global_counter;
  Shared() : have_port(false), counter(0), global_counter(0) {}
  bool Get(string& ip, string& port) const {
    unique_lock<mutex> lock(mx);
    ip = from_ip;
    port = from_port;
		return have_port;
  }
  void Update(const string& ip, const string& port) {
    unique_lock<mutex> lock(mx);
		have_port = true;
    if (from_ip != ip || from_port != port) { cout << format("Port change: %s %s\n") % ip % port; }
    from_ip = ip;
    from_port = port;
  }

  shared_ptr<SX1276Radio> radio;
  shared_ptr<SX1276Platform> platform;
  mutex radio_mx;

	bool TransmitHello() {
		// radio->SetPreamble(0x80);
		for (int i=0; i < 5; i++) {
			uint8_t buffer[9] = { 0x02, 0x02, (global_counter++) & 0xff, 'h', 'e', 'l', 'l', 'o', 0x30+i};
	    radio->SendSimpleMessage(buffer, 9);
			usleep(100000); // Not too close, we do miss them (maybe still a bug in rx)
		}
	}

	bool TransmitAck(uint8_t counter) {
		cerr << format("Send ACK %d\n") % (int)counter;
		// radio->SetPreamble(0x80);
		// FIXME radio->SetSymbolTimeout(0x80);
		int i=0;
		uint8_t buffer[9] = { 0x01, counter, (global_counter++) & 0xff, 1, 2, 3, 4, 5, i+0x30 };
    radio->SendSimpleMessage(buffer, 9);
	}

	void Init() {
    platform->ResetSX1276();
    radio->ChangeCarrier(919000000);
    radio->ApplyDefaultLoraConfiguration();
	}

	void Restart() {
    radio->Standby();
    Init();
	}

  // This stuff is a hack   :needs to be abstracted into a thread safe radio instance
  bool Transmit(const void* payload, unsigned len) {
    unique_lock<mutex> lock(radio_mx);

		uint8_t buffer[len+3]; // first byte: 00 = MQTT-SN, 01 = ACK second byte: counter; 02 = HELLO, 3rd = global counter
		buffer[0] = 0;
		buffer[1] = counter & 0xff;
		memcpy(buffer+3, payload, len);

		// So first problem: message not getting through when only sent once.
		// Needs further investigation, there is a lot of stuff in the PDF I still havent addressed yet
		// So lets put a layer over the top: very Dodgy Bros ACK / timeout thing
	  steady_clock::time_point t0 = steady_clock::now();
	  steady_clock::time_point t1 = t0 + boost::chrono::milliseconds(15000);
		int tries = 0;
		// radio->SetPreamble(0x50); // probably a red herring now I found the RX bug
		for (; steady_clock::now() <= t1; ) {
			buffer[2] = (global_counter++) & 0xff;

			if (tries % 15 == 0) { // resend every 5 missed ack for the moiment
			  if (!radio->SendSimpleMessage(buffer, len+3)) {
					break;
				}
			}
			// Wait for an ack then try again

			uint8_t ack_buffer[256];
			bool rx_timeout = false;
			bool rx_crc_error = false;
			int received = 255;
      if (radio->ReceiveSimpleMessage(ack_buffer, received, 3000, rx_timeout, rx_crc_error)) {
				// ACK packet: 01 + counter echo
				if (!rx_timeout && !rx_crc_error) {
					if (received == 10 && ack_buffer[0] == 1 && ack_buffer[1] == counter) {
						cerr << format("ACK OK %d\n") % (int)counter;
						counter ++;
						return true;
					}
					cerr << format("Waiting for ACK; got other data: %.2x %.2x\n") % int(ack_buffer[0]) % int(ack_buffer[1]);

					// This is where it gets really really dodgy
					// we get a 'deadlock' unless we ack this message
					// for now then, though, discard it after acking
					// and let MQTT-SN sort out the mess
					if (ack_buffer[0] == 0) {
						TransmitAck(ack_buffer[1]);
					}
				}
				else {
				  //if (rx_timeout) { cerr << "to "; }
					if (rx_crc_error) { cerr << "crc\n"; }
				}
				tries++;
			}
		}
		cerr << format("Failed to send msg and get ack, counter = %u tried %d times\n") % (unsigned)counter % tries;
    counter ++;
  }
  bool TryReceive(uint8_t* payload, unsigned len, unsigned& rx) {

    // Do a blocking receive, but in such a way we can break it out if Transmit needs to do its thing
    // OTOH dont let a high rate of TX starve receiving

    bool crc_error = false;
    bool timeout = false;
    unsigned timeout_ms = 20000; // This timeout is a safety sanity check; ReceiveSimpleMessage returns on a symbol timeout without retrying

    unique_lock<mutex> lock(radio_mx);
    int received = 0;
		uint8_t buffer[len+3];
    do {
      received = len+3;
      if (!radio->ReceiveSimpleMessage(buffer, received, timeout_ms, timeout, crc_error)) { return false; }

			if (!timeout && !crc_error) {

//	      FILE* f = popen("od -Ax -tx1z -v -w16", "w");
//	      if (f) { fwrite(buffer, received, 1, f); pclose(f); }

				if (buffer[0] == 2) { cout << "[RX Hello]\n"; continue; } // hello msg, ignore
				if (buffer[0] == 1) { cout << "[RX ACK]\n"; continue; } // ack, ignore (for the moment)
				if (buffer[0] == 0) {
					TransmitAck(buffer[1]);
					memcpy(payload, buffer+3, received-3);
					rx = received-3;
					return true;
				}
			}

      // allow pending tx...
      lock.unlock();
      usleep(50); // there must be a better way to do this...
      lock.lock();
    } while (true); // DODGY: we need a higher level time timeout for O/S servicing
		cerr << format("Junk? %.2x\n") % buffer[0];
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
        cerr << format("[UDP RX] %s:%d : %d:%s\n") % from % fromport % n % util::buf2str(buffer,n);

	      FILE* f = popen("od -Ax -tx1z -v -w16", "w");
	      if (f) { fwrite(buffer, n, 1, f); pclose(f); }

        { shared_.Update(from, fromport); }

        if (!shared_.Transmit(buffer, n)) {
	        cerr << "TX error!\n";
				}
        cerr << format("[UDP RX] FIN\n");

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
      bool ok = shared_.TryReceive(buffer, 256, r);

      if (ok) { // FIXME
        string ip; string port;
        bool have_port = shared_.Get(ip, port);
				if (!have_port) { cerr << format("[Radio RX -> NOWHERE] %d:%s\n") % r % util::buf2str(buffer,r); }
				else {
        cerr << format("[Radio RX -> %s:%s] %d:%s\n") % ip % port % r % util::buf2str(buffer,r);
	      FILE* f = popen("od -Ax -tx1z -v -w16", "w");
	      if (f) { fwrite(buffer, r, 1, f); pclose(f); }
        try {
          srv_.sndto(buffer, r, ip, port);
        } catch (libsocket::socket_exception& e) { cerr << e.mesg<< "\n"; }
				}
      } else {
		    shared_.Restart();
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

  if (argc < 3) { fprintf(stderr, "Usage: %s <spidev> <udp-listen-port>\n", argv[0]); return 1; }

  if ((port = atoi(argv[2]) < 1)) { cerr << "Invalid port.\n"; return 1; }

  libsocket::inet_dgram_server srv(UDP_BIND_IP, argv[2], LIBSOCKET_IPv4);

  shared_ptr<SX1276Platform> platform = SX1276Platform::GetInstance(argv[1]);
  if (!platform) { PR_ERROR("Unable to create platform instance\n"); return 1; }

  shared_ptr<SPI> spi = platform->GetSPI();
  if (!spi) { PR_ERROR("Unable to get SPI instance\n"); return 1; }

  usleep(100);
  shared_ptr<SX1276Radio> radio(new SX1276Radio(spi));
  cout << format("SX1276 Version: %.2x\n") % radio->version();

  radio->SetPreamble(0x50); // probably a red herring now I found the RX bug

  Shared shared;
  shared.radio = radio;
  shared.platform = platform;

	shared.Init();
  cout << format("Carrier Frequency: %uHz\n") % radio->carrier();
  if (shared.radio->fault()) { PR_ERROR("Radio Fault\n"); return 1; }

  WorkerThread outThread(srv, shared, true);
  WorkerThread inThread(srv, shared, false);

	shared.TransmitHello();

  thread outInstance(boost::bind(&WorkerThread::Run, &outThread));
  thread inInstance(boost::bind(&WorkerThread::Run, &inThread));
  outInstance.join();
  inInstance.join();
  cout << "DONE\n";


}

