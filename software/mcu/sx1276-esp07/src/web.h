#ifndef SENTRIFARM_WEB_H__
#define SENTRIFARM_WEB_H__

#include <memory>

class ESP8266WebServer;

namespace sentrifarm {

// GET http://gateway        <-- status page
// GET http://gateway/reset  <-- API call: drop wifi configuration, reset back into AP + captive portal

class WebServer
{
public:
  void begin();
  void handle();

  void setStatus(String statusMsg) { statusMsg_ = statusMsg; }

private:
  std::unique_ptr<ESP8266WebServer> webServer_;

  void handleRoot();
  void handleReset();
  void handleNotFound();

  String statusMsg_;
};

}

#endif
