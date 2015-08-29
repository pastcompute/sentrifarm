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

#include "Arduino.h"
#include <SPI.h>
#include <Wire.h>
// Note: for some reason, platformio will fail to add the path to this files include directory to its library if not #include'd by the .ino file
#include <mqttsn.h>
#include <sx1276.h>
#include <sx1276mqttsn.h>
#include "sf-mcu.h"
#include "sf-ioadaptorshield.h"
#include "sf-sensordata.h"
#include "sf-bmp180.h"
#include "sf-pcf8591.h"
#include "sf-ds1307.h"
#include <Adafruit_BMP085_U.h>

#ifdef ESP8266
#include "ESP8266WiFi.h"
#endif

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

SPISettings spiSettings(1000000, MSBFIRST, SPI_MODE0);

SX1276Radio radio(PIN_SX1276_CS, spiSettings);

MQTTSX1276 MQTTHandler(radio);

Sentrifarm::SensorData sensorData;

enum PushStates {
  NEED_CONNECT,
  SENT_CONNECT,
  WAIT_REGACK,
  WAIT_PUBACK
};

#define FAULT_SLEEP_INTERVAL_MS 20000

#define ROUTINE_SLEEP_INTERVAL_MS 60000

#define STATS_INTERVAL_MS 6000

struct Metrics
{
  int rx_count;
  int crc_count;
  int timeout_count;

  void reset() {
    rx_count = 0;
    crc_count = 0;
    timeout_count = 0;
  }
};

PushStates state = NEED_CONNECT;

Metrics metrics;

elapsedMillis elapsedRuntime;

uint16_t registered_topic_id = 0xffff;

void read_chip_once()
{
  sensorData.chipVersion = ESP.getBootVersion();
  sensorData.chipBootMode = ESP.getBootMode();
  sensorData.chipVcc = ESP.getVcc();
  snprintf(sensorData.sdk, sizeof(sensorData.sdk), ESP.getSdkVersion());

  // Now, we could use the eeprom to save some kind of id
  // for the moment, use the MAC of the ESP8266
#ifdef ESP8266
  WiFi.macAddress(sensorData.mac);
#endif

}

void read_radio_once()
{
  SPI.begin();
  sensorData.radio_version = radio.ReadVersion();
  SPI.end();
  Serial.println(sensorData.radio_version);
  if (sensorData.radio_version != 0 && sensorData.radio_version != 0xff) {
    sensorData.have_radio = true;
  }
}

void boot_count()
{
  // Update bootcount in nvram
  byte b0 = Sentrifarm::read_nvram_byte(10);
  byte b1 = Sentrifarm::read_nvram_byte(11);
  byte b2 = Sentrifarm::read_nvram_byte(12);
  byte b3 = Sentrifarm::read_nvram_byte(13);
  uint32_t bc = (uint32_t(b0) << 24) | (uint32_t(b1) << 16) | (uint32_t(b2) << 8) | uint32_t(b3);
  bc ++;
#if 0
  // We are going to need to work out a better way than loading a one-off firmware...
  // Although having a board where you plug in the rtc, boot to zero, then plug into the real board
  // is probably not completely bad idea
  bc = 0;
#endif

  sensorData.bootCount = bc;
  Sentrifarm::save_nvram_byte(10, (bc & 0xff000000) >> 24);
  Sentrifarm::save_nvram_byte(11, (bc & 0xff0000) >> 16);
  Sentrifarm::save_nvram_byte(12, (bc & 0xff00) >> 8);
  Sentrifarm::save_nvram_byte(13, (bc & 0xff));
}

// --------------------------------------------------------------------------
void setup()
{
  Sentrifarm::setup_world(F("LEAF Node V0.1"));
  Sentrifarm::setup_shield();
  Sentrifarm::led4_on();
  Sentrifarm::reset_radio();

  // 0x48 (ADC), 0x50 (EEPROM), 0x68 (RTC), 0x77 (BMP)
  // For reasons I dont understand, the RTC is only working if we first scan the entire i2c bus. WTAF?
  Wire.begin();  Sentrifarm::scan_i2c_bus();

  Serial.println(F("go..."));
  Sentrifarm::led4_double_short_flash();

  // So, to keep things simple, we read all the sensors now,
  // then use loop() as a state machine processor for publishing the data to the central node
  memset(&sensorData, 0, sizeof(sensorData)); // probably redundant
  sensorData.reset();

  read_chip_once();
  boot_count();
  Sentrifarm::read_datetime_once(sensorData);
  Sentrifarm::read_bmp_once(sensorData, bmp);
  Sentrifarm::read_pcf8591_once(sensorData);
  read_radio_once();

  sensorData.debug_dump();

  Sentrifarm::led4_double_short_flash();

  // This also initialises correct carrier frequency, etc.
  MQTTHandler.Begin(&Serial);

  metrics.reset();

  elapsedRuntime = 0;

  // Make the first connect attempt
  MQTTHandler.connect(0, 30, "sfnode1"); // keep alive in seconds
  state = SENT_CONNECT;
}

// void loop() {}

// --------------------------------------------------------------------------
bool register_topic()
{
  const char TOPIC[] = "sentrifarm/leaf/data";
  uint16_t topic_id = 0xffff;
  uint8_t idx = 0;
  if (0xffff == (topic_id = MQTTHandler.find_topic_id(TOPIC, idx))) {
    Serial.println("Try register");
    if (!MQTTHandler.register_topic(TOPIC)) {
      Serial.println("Register error");
      Sentrifarm::deep_sleep_and_reset(FAULT_SLEEP_INTERVAL_MS);
      return false;
    }
    return false;
  }
  // registered
  registered_topic_id  = topic_id;
  return true;
}

void publish_data()
{
  char buf[192]; // keep it short...

  // For the moment send ASCII
  sensorData.make_mqtt_0(buf, sizeof(buf));

  MQTTHandler.publish(FLAG_QOS_1, registered_topic_id , buf, strlen(buf));
}

// --------------------------------------------------------------------------
void loop()
{
  // Sentrifarm::deep_sleep_and_reset(10000);

  // See if we can receive any radio data
  bool rx_ok = false;
  bool crc = false;
  if (MQTTHandler.TryReceive(crc)) {
    metrics.rx_count ++;
    rx_ok = true;
    Sentrifarm::led4_flash();
  } else if (crc) { metrics.crc_count++; } else { metrics.timeout_count++; }

  if (elapsedRuntime > STATS_INTERVAL_MS) {
    Serial.print(" RX count: "); Serial.print(metrics.rx_count);
    Serial.print(" Timeout count: "); Serial.print(metrics.timeout_count);
    Serial.print(" CRC count: "); Serial.print(metrics.crc_count);
    Serial.println();
    elapsedRuntime = 0;
  }

  // Last msg rx'd was corrupted, so ignore it
  if (!rx_ok) { return; }

  // Deal with timeouts and resends
  MQTTHandler.ResetDisconnect();
  if (MQTTHandler.wait_for_response()) {
    if (MQTTHandler.DidDisconnect()) {
      // We got a MQTTSN disconnect for some unexplained reason
      return;
    }
    // timeout - try again
    return;
  }

  switch (state) {
    case SENT_CONNECT:
      // if we got here, then we processed a CONNACK.
      // register our topic
      register_topic();
      // We expect the next message to be a regack
      state = WAIT_REGACK;
      return;

    case WAIT_REGACK:
      publish_data();
      state = WAIT_PUBACK;
      break;

    case WAIT_PUBACK:
      Sentrifarm::deep_sleep_and_reset(ROUTINE_SLEEP_INTERVAL_MS);
      break;

    default:
      break;
  }
}
