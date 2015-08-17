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

class RadioManager
{
public:
  RadioManager(shared_ptr<SX1276Radio>& radio, shared_ptr<SX1276Platform>& platform)
  :  radio_(radio),
    platform_(platform),
    have_port_(false), rolling_counter_(0), rolling_counter_rx_(0xff),
    num_tx_(0), num_valid_received_(0), num_crc_errors_(0), num_junk_(0), num_xorv_(0), dropped_(0),
    have_rx_(false)
  {
  }
  void Restart() {
    unique_lock<mutex> lock(radio_mutex_);
    platform_->ResetSX1276();
    radio_->ChangeCarrier(919000000);
    radio_->ApplyDefaultLoraConfiguration();
  }
  bool GetPort(string& ip, string& port) const {
    unique_lock<mutex> lock(port_mutex_);
    if (have_port_) {
      ip = from_ip_;
      port = from_port_;
    }
    return have_port_;
  }
  void SetPort(const string& ip, const string& port) {
    unique_lock<mutex> lock(port_mutex_);
    if (from_ip_ != ip || from_port_ != port) {
      cout << format("Port change: %s %s\n") % ip % port;
      // TODO: break out of receive early?
    }
    have_port_ = true;
    from_ip_ = ip;
    from_port_ = port;
  }

  // Very dumb layer 2 protocol
  // Byte 0 : 0x00 == MQTT-SN data, 0x02 == HELLO message
  // Byte 1 : Rolling counter (for debug purposes)
  // Byte 2 : Rolling counter last received from other side, for debug purposes
  // Byte N : xor of rest of buffer - because CRC can pass but data get corrupted by BusPirate serial it seems
  bool TransmitHello() {
    unique_lock<mutex> lock(radio_mutex_);
    for (int i=0; i < 5; i++) {
      uint8_t buffer[10] = { 0x02, rolling_counter_, 0xff, 'h', 'e', 'l', 'l', 'o', (uint8_t)('0'+i), 0};
      uint8_t xorv = 0;
      for (unsigned j=0; j < 9; j++) {
        xorv = xorv ^ buffer[j];
      }
      buffer[9] = xorv;
      rolling_counter_ ++;
      radio_->SendSimpleMessage(buffer, sizeof(buffer));
      usleep(100000); // Not too close, sometimes they dont all get received
    }
  }
  bool Transmit(const void* payload, unsigned len) {

    uint8_t buffer[len+4];
    buffer[0] = 0x0;
    buffer[1] = rolling_counter_;
    memcpy(buffer+3, payload, len);
    rolling_counter_ = (rolling_counter_==0xff ? 0 : rolling_counter_+1);

    unique_lock<mutex> lock(radio_mutex_);
    buffer[2] = rolling_counter_rx_;

    uint8_t xorv = 0;
    for (unsigned j=0; j < len+3; j++) {
      xorv = xorv ^ buffer[j];
    }
    buffer[len+3] = xorv;
#if 1
	  cout << "Predicted time on air: " << radio_->PredictTimeOnAir(buffer, sizeof(buffer)) << "\n";
#endif
    if (!radio_->SendSimpleMessage(buffer, sizeof(buffer))) {
      // SPI error
      return false;
    }
    num_tx_++;
    return true;
  }
  void PrintStats() {
    cout << format("TX=%4u RX=%4u CRC=%4u JUNK=%4u DROPPED=%d\n") % num_tx_ % num_valid_received_ % num_crc_errors_ % num_junk_ % dropped_;
  }
  bool TryReceive(uint8_t* payload, unsigned len, unsigned& rx)
  {
    // Do a blocking receive, but in such a way we can break it out if Transmit needs to do its thing
    // OTOH dont let a high rate of TX starve receiving
    bool crc_error = false;
    bool timeout = false;
    unsigned timeout_ms = 20000; // This timeout is a safety sanity check; normally, ReceiveSimpleMessage returns on a symbol timeout without retrying

    unique_lock<mutex> lock(radio_mutex_);
    int received = 0;
    uint8_t buffer[len+4];
    do {
      received = sizeof(buffer);
      if (!radio_->ReceiveSimpleMessage(buffer, received, timeout_ms, timeout, crc_error)) {
        // SPI error
        return false;
      }
      if (!timeout && !crc_error) {
        uint8_t xorv = 0;
        for (unsigned j=0; j < received - 1; j++) {
          xorv = xorv ^ buffer[j];
        }
        if (xorv != buffer[received-1]) {
          cerr << format("XOR checksum error! %.2x != %.2x\n") % (int)xorv % (int)buffer[received-1];
          num_xorv_ ++;
          FILE* f = popen("od -Ax -tx1z -v -w16", "w");
          if (f) { fwrite(buffer, received, 1, f); pclose(f); }
        }
        else if (buffer[0] == 2) {
          rolling_counter_rx_ = buffer[1];
          have_rx_ = false;
          cout << format("[RX Hello] cntr=%d\n") % (int)buffer[1];
          continue;
        }
        else if (buffer[0] == 0) {
          num_valid_received_ ++;
          PrintStats();
          uint8_t received_counter = buffer[1];
          uint8_t expected_counter = (rolling_counter_rx_ == 0xff ? 0 : rolling_counter_rx_+1);
          if (!have_rx_) {
            have_rx_ = true;
          }
          else if (received_counter != expected_counter) {
            // if received > expected then a message got lost
            int skipped = (int)received_counter - (int)expected_counter;
            if (skipped < 1) { skipped += 256; }
            cerr << format("Dropped %d messages? cntr.xpt=%d cntr.rxd=%d othr.rxd=%d\n") % skipped % (int)expected_counter % (int)buffer[1] % (int)buffer[2];
            dropped_ += skipped;
          }
          rolling_counter_rx_ = buffer[1];
          memcpy(payload, buffer+3, received-4);
          rx = received-4;
          return true;
        } else {
          num_junk_ ++;
          cerr << format("Junk? type=%.2x cntr=%d\n") % (int)buffer[0] % (int)buffer[1];
        }
      } else if (crc_error) {
        num_crc_errors_ ++;
        cerr << "CRC error\n";
      }

      // allow pending tx...
      lock.unlock();
      usleep(50); // there must be a better way to do this...
      lock.lock();
    } while (true); // DODGY: we need a higher level time timeout for O/S servicing
  }
private:
  shared_ptr<SX1276Radio> radio_;
  shared_ptr<SX1276Platform> platform_;
  mutex radio_mutex_;          ///< Protect access to the radio
  string from_ip_;             ///< IP last UDP packet was received from
  string from_port_;           ///< port last UDP packet was received from
  mutable mutex port_mutex_;   ///< Protection for from_ip_, from_port_
  bool have_port_;             ///< false until from_port_ set for the first time
  uint8_t rolling_counter_;    ///< Rolling message counter output
  uint8_t rolling_counter_rx_; ///< Rolling message counter last received
  int num_tx_;                 ///< Number of transmitted MQTT-SN messages
  int num_valid_received_;     ///< Number of valid received MQTT-SN messages
  int num_crc_errors_;         ///< Number of crc errors
  int num_junk_;               ///< Number of junk messages
  int num_xorv_;               ///< Number of junk XOR messages
  int dropped_;                ///< Estimated number of lost messages in transit
  bool have_rx_;               ///< false until first message received successfully
};

class WorkerThread
{
  libsocket::inet_dgram_server& srv_;
  RadioManager& radio_;
  bool outward_;
  void OutLoop() {
    for (;;) {
      uint8_t buffer[127];
      string from, fromport;
      int n = srv_.rcvfrom(buffer, sizeof(buffer), from, fromport);
      if (n > 0) {
        { radio_.SetPort(from, fromport); }
        cerr << format("[UDP RX] %s:%d : %d:%s\n") % from % fromport % n % util::buf2str(buffer,n);

        FILE* f = popen("od -Ax -tx1z -v -w16", "w");
        if (f) { fwrite(buffer, n, 1, f); pclose(f); }


        if (!radio_.Transmit(buffer, n)) {
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
      bool ok = radio_.TryReceive(buffer, 256, r);

      if (ok) { // FIXME
        string ip; string port;
        bool have_port = radio_.GetPort(ip, port);
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
        radio_.Restart();
      }
    }
  }
public:
  // TODO: abstrfact SX1276 RADIo to Radio, etc
  WorkerThread(libsocket::inet_dgram_server& srv, RadioManager& radio, bool outward)
  : srv_(srv),
    radio_(radio),
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

  // radio->SetPreamble(0x50); // probably a red herring now I found the RX bug

  RadioManager radio_manager(radio, platform);
  radio_manager.Restart();
  cout << format("Carrier Frequency: %uHz\n") % radio->carrier();
  if (radio->fault()) { PR_ERROR("Radio Fault\n"); return 1; }

  WorkerThread outThread(srv, radio_manager, true);
  WorkerThread inThread(srv, radio_manager, false);

  radio_manager.TransmitHello();

  thread outInstance(boost::bind(&WorkerThread::Run, &outThread));
  thread inInstance(boost::bind(&WorkerThread::Run, &inThread));
  outInstance.join();
  inInstance.join();
  cout << "DONE\n";
}

