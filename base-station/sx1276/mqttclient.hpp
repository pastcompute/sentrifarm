#ifndef MQTT_HPP__
#define MQTT_HPP__

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

/// Class encapsulating MQTT operations.
/// Presently this is implemented using mosquitto, however this may change, or need to be
/// changed in the future for various reasons.
/// The client is immutable and thus doesnt make sense to copy an instance.
class MQTTClient : boost::noncopyable
{
public:
  /// @param client_id MQTT client id. This class is designed such that the id is immutable.
  MQTTClient(const char *client_id);
  ~MQTTClient();

  bool Connect();
  bool Subscribe(const char *topic);
  bool Unsubscribe(const char *topic);

private:

  std::string client_id_;
  class MQTTClientImpl;
  boost::shared_ptr<MQTTClientImpl> impl_;
};

#endif // MQTT_HPP__
