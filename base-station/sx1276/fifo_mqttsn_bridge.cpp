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
#include <boost/thread.hpp>
#include "libsocket/inetserverdgram.hpp"
#include "util.hpp"
#include <unistd.h>
#include <sys/fcntl.h>

using std::string;
using std::cout;
using std::cerr;
using boost::format;
using boost::thread;
using boost::mutex;
using boost::unique_lock;

struct Shared
{
  mutable mutex mx;
  string from_ip;
  string from_port;
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
};

class WorkerThread
{
  libsocket::inet_dgram_server& srv_;
  int fd_;
  Shared& shared_;
  bool outward_;
  void OutLoop() {
    for (;;) {
      uint8_t buffer[512];
      string from, fromport;
      int n = srv_.rcvfrom(buffer, sizeof(buffer), from, fromport);
      if (n > 0) {
        // loop in case fd is a pipe or something else odd
        cerr << format("[R] %s:%d : %s\n") % from % fromport % util::buf2str(buffer,n);
        { shared_.Update(from, fromport); }
        uint8_t* p = buffer;
        while (n > 0) {
          int r = write(fd_, p, n);
          if (r > 0) { n-= r; p+=r; }
          else if (r == 0) { /* should not happen... */ }
          else { cerr << util::safe_perror(errno, "Unable to write") << "\n"; return; }
        }
      }
    }
  }
  void InLoop() {
    for (;;) {
      uint8_t buffer[512];
      int r = read(fd_, buffer, 512);
      if (r < 0) {
        if (errno != EBADF) { cerr << util::safe_perror(errno, "Unable to read") << "\n"; } }
      else if (r > 0) {
        string ip; string port;
        shared_.Get(ip, port);
        cerr << format("[S] %s:%s %s\n") % ip % port % util::buf2str(buffer,r);
        try {
          srv_.sndto(buffer, r, ip, port);
        } catch (...) {}
      } else {
//        cout << "punt\n";
      }
    }
  }
public:
  WorkerThread(libsocket::inet_dgram_server& srv, int fd, Shared& shared, bool outward)
  : srv_(srv),
    fd_(fd),
    shared_(shared),
    outward_(outward)
  {}
  void Run() {
    cout << format("Run %s fd=%d\n") % (outward_ ? "out" : "in") % fd_;
    try {
      if (outward_) { OutLoop(); } else { InLoop(); }
    } catch (const libsocket::socket_exception& exc) {
      cerr << exc.mesg;
    }
    cout << "Run Finish\n";
  }
};

// $1 == RSMB MQTT-SN broker bridge port
// $2 == path to write to
// $3 == path to read from
int main(int argc, char *argv[])
{
  int port = 1884;
  if (argc < 5) { cerr << format("%s <ip> <port> <out-path> <in-path>\n") % argv[0]; return 1; }
  if ((port = atoi(argv[2]) < 1)) { cerr << "Invalid port.\n"; return 1; }

  int fd_out = open(argv[3], O_NOATIME | O_NOCTTY | O_WRONLY);
  if (fd_out == -1) { cerr << "Unable to access out-path\n"; }

  int fd_in = open(argv[4], O_NOATIME | O_NOCTTY);
  if (fd_in == -1) { cerr << "Unable to access in-path\n"; }

  libsocket::inet_dgram_server srv(argv[1], argv[2], LIBSOCKET_BOTH);

  Shared shared;
  WorkerThread outThread(srv, fd_out, shared, true);
  WorkerThread inThread(srv, fd_in, shared, false);

  thread outInstance(boost::bind(&WorkerThread::Run, &outThread));
  thread inInstance(boost::bind(&WorkerThread::Run, &inThread));
  outInstance.join();
  inInstance.join();
  cout << "DONE\n";
}

