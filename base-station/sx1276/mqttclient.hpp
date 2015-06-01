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
#ifndef MQTT_HPP__
#define MQTT_HPP__

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>

/// Class encapsulating MQTT operations.
/// Designed for simple use cases, in particular, where there is a broker locally or on a low latency link.
///
/// Presently this is implemented using mosquitto, however this may change, or need to be
/// changed in the future for various reasons, the class is designed in such a way that all changes are isolated to mqttclient.cpp.
///
/// The client is immutable and thus doesnt make sense to copy an instance.
/// For the moment assume methods are not re-entrant.
class MQTTClient : boost::noncopyable
{
public:
  typedef boost::function<void(const char*, const char*,const void*,unsigned)> message_fcn_t;
  typedef boost::function<void(const char*, bool)> connack_fcn_t;

  virtual ~MQTTClient();

  /// Create an instance of MQTTClient, using specified broker
  /// @param client_id MQTT client id
  /// @param host broker host; NULL defaults to 127.0.0.1
  /// @param id broker port
  static boost::shared_ptr<MQTTClient> CreateInstance(const char *client_id, const char *host=NULL, int port=1883);

  bool valid() const { return valid_; }
  const char *client_id() const { return client_id_.c_str(); }
  const char *last_error() const { return last_error_.c_str(); }
  virtual bool have_connack() const = 0;

  /// Attempt to connect to MQTT broker.
  /// Success does not guarantee broker session; check have_connack() to confirm after calling Poll()
  /// @return true if network connection to broker made
  virtual bool Connect() = 0;

  /// Subscribe to a particular topic.
  /// @param topic Topic to subscribe
  /// @return false if not connected to broker
  virtual bool Subscribe(const char *topic) = 0;

  /// Unsubscribe from a particular topic.
  /// @param topic Topic to unsubscribe
  /// @return false if not connected to broker
  virtual bool Unsubscribe(const char *topic) = 0;

  /// Check for network activity and process any incoming messages.
  /// If the connection to the broker was dropped will try and reconnect automatically
  virtual bool Poll() = 0;

  // We could probably go to town and have a function specific for a topic, but for now we leave that for a rainly day
  inline void RegisterMessageHandler(const message_fcn_t& handler);
  inline void RegisterConnackHandler(const connack_fcn_t& handler);

protected:
  /// Constructor. Set up any underlying library resouces, etc.
  /// If operation fails, ok() returns false and all other methods will fail.
  /// @param client_id MQTT client id for this instance.
  /// @param broker_host hostname / ip address of broker
  /// @param broker_port port for broker
  MQTTClient(const char *client_id, const char *broker_host, int broker_port);

  bool valid_;
  std::string client_id_;
  std::string broker_host_;
  std::string last_error_;
  int broker_port_;
  unsigned keep_alive_s_;
  message_fcn_t message_fcn_;
  connack_fcn_t connack_fcn_;

private:
};

inline MQTTClient::MQTTClient(const char *client_id, const char *host, int port)
  : valid_(false),
    client_id_(client_id),
    broker_host_(host), broker_port_(port), keep_alive_s_(60)
{
}

inline MQTTClient::~MQTTClient()
{
}


inline void MQTTClient::RegisterMessageHandler(const message_fcn_t& handler)
{
  message_fcn_ = handler;
}

inline void MQTTClient::RegisterConnackHandler(const connack_fcn_t& handler)
{
  connack_fcn_ = handler;
}

#endif // MQTT_HPP__
