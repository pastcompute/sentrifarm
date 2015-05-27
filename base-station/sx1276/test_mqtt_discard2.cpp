#include "mqttclient.hpp"
#include <string>
#include <iostream>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>

using std::string;
using std::cout;
using std::cerr;
using boost::format;
using boost::shared_ptr;

int main(int argc, char *argv[])
{
  MQTTClient client(argv[0]);
}
