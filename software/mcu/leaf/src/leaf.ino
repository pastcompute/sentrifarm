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

#define WITH_DHT 1
#if WITH_DHT
#include <DHT.h>
#endif

#ifdef ESP8266
#include "ESP8266WiFi.h"
extern "C" {
#include "user_interface.h"
}
#endif

#ifdef TEENSYDUINO
#define Serial Serial1
#endif

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

SPISettings spiSettings(1000000, MSBFIRST, SPI_MODE0);

SX1276Radio radio(PIN_SX1276_CS, spiSettings);

MQTTSX1276 MQTTHandler(radio);

Sentrifarm::SensorData sensorData;

#if WITH_DHT
DHT HumiditySensor(PIN_DHT,DHT11);
#endif

enum PushStates {
  NEED_CONNECT,
  SENT_CONNECT,
  WAIT_REGACK,
  WAIT_PUBACK
};

#define FAULT_SLEEP_INTERVAL_MS 20000

#define ROUTINE_SLEEP_INTERVAL_MS 60000

#define STATS_INTERVAL_MS 6000

#define RUNTIME_TIMEOUT 20000

struct Metrics
{
  int rx_count;
  int crc_count;
  int timeout_count;
  int disconnect;

  void reset() {
    rx_count = 0;
    crc_count = 0;
    timeout_count = 0;
    disconnect = 0;
  }
};

PushStates state = NEED_CONNECT;

Metrics metrics;

elapsedMillis elapsedRuntime;
elapsedMillis elapsedStatTime;

uint16_t registered_topic_id = 0xffff;

bool in_beacon_mode = false;
bool in_log_mode = false;

int32_t beacon_counter = 0;

ICACHE_FLASH_ATTR
void read_chip_once()
{
#ifdef ESP8266
  sensorData.chipVersion = ESP.getBootVersion();
  sensorData.chipBootMode = ESP.getBootMode();
  sensorData.chipVcc = ESP.getVcc();
  snprintf(sensorData.sdk, sizeof(sensorData.sdk), ESP.getSdkVersion());

  // Now, we could use the eeprom to save some kind of id
  // for the moment, use the MAC of the ESP8266
  WiFi.macAddress(sensorData.mac);
  sensorData.have_mac = true;
#endif
}

ICACHE_FLASH_ATTR
void read_radio_once()
{
  SPI.begin();
  sensorData.radio_version = radio.ReadVersion();
  SPI.end();
  Serial.println(sensorData.radio_version);
  if (sensorData.radio_version != 0 && sensorData.radio_version != 0xff) {
    sensorData.have_radio = true;
    sensorData.rssi = 0;
  }
}

ICACHE_FLASH_ATTR
void boot_count()
{
  // Update bootcount in nvram
  byte b0 = Sentrifarm::read_nvram_byte(10);
  byte b1 = Sentrifarm::read_nvram_byte(11);
  byte b2 = Sentrifarm::read_nvram_byte(12);
  byte b3 = Sentrifarm::read_nvram_byte(13);
  uint32_t bc = (uint32_t(b0) << 24) | (uint32_t(b1) << 16) | (uint32_t(b2) << 8) | uint32_t(b3);
  bc ++;

  // Use beacon mode to reset the boot count to zero
  if (in_beacon_mode) {
    bc = 0;
  }

  sensorData.bootCount = bc;
  Sentrifarm::save_nvram_byte(10, (bc & 0xff000000) >> 24);
  Sentrifarm::save_nvram_byte(11, (bc & 0xff0000) >> 16);
  Sentrifarm::save_nvram_byte(12, (bc & 0xff00) >> 8);
  Sentrifarm::save_nvram_byte(13, (bc & 0xff));
}

// --------------------------------------------------------------------------
void setup()
{
  // So, to keep things simple, we read all the sensors now,
  // then use loop() as a state machine processor for publishing the data to the central node
  memset(&sensorData, 0, sizeof(sensorData)); // probably redundant
  sensorData.reset();

  Sentrifarm::setup_world(F("LEAF Node V0.1"));
  Sentrifarm::setup_shield(in_beacon_mode, in_log_mode);
  Sentrifarm::led4_on();
  Sentrifarm::reset_radio();

  if (in_beacon_mode && !in_log_mode) {
    Serial.println(F("BEACON MODE"));
    sensorData.beacon_mode = true;
  } else if (in_log_mode && !in_beacon_mode) {
    Serial.println(F("RADIOLOG MODE"));
  } else {
#ifdef ESP8266
  // copy the counter into the ESP nvram
    system_rtc_mem_write(64, &beacon_counter, 4);
#endif
  }

  if (!in_beacon_mode && !in_log_mode) {
    Wire.begin();
    // 0x48 (ADC), 0x50 (EEPROM), 0x68 (RTC), 0x77 (BMP)
    // For reasons I dont understand, the RTC is only working if we first scan the entire i2c bus. WTAF?
    // Note: this also hangs if the RTC is plugged in wrong
#if !defined(TEENSYDUINO)
    Sentrifarm::scan_i2c_bus();
#endif
  }

  Serial.println(F("go..."));
  Sentrifarm::led4_double_short_flash();

  read_chip_once();
  boot_count();
  if (!in_beacon_mode && !in_log_mode) {
    Sentrifarm::read_datetime_once(sensorData);
    Sentrifarm::read_bmp_once(sensorData, bmp);
    Sentrifarm::read_pcf8591_once(sensorData);
#if WITH_DHT
    HumiditySensor.begin();
    bool hh = HumiditySensor.read();
    if (hh) {
      sensorData.humidity = HumiditySensor.readHumidity();
      sensorData.have_humidity = true;
      Serial.print("H/T ");
      Serial.print(sensorData.humidity);
      Serial.print(",");
      Serial.print(HumiditySensor.readTemperature());
      Serial.println();
    }
    digitalWrite(PIN_DHT, HIGH);
#endif
  }
  read_radio_once();

  sensorData.debug_dump();

  Sentrifarm::led4_double_short_flash();

  // This also initialises correct carrier frequency, etc.
  MQTTHandler.Begin(&Serial);

  metrics.reset();

  elapsedRuntime = 0;
  elapsedStatTime = 0;

  if (in_beacon_mode) {
    return;
  }

  if (in_log_mode) {
    return;
  }

  // Make the first connect attempt
  // FIXME should use mac or something
  MQTTHandler.connect(0, 30, "sfnode1"); // keep alive in seconds
  state = SENT_CONNECT;
  Sentrifarm::led4_double_short_flash();
}

// void loop() {}

// --------------------------------------------------------------------------
ICACHE_FLASH_ATTR
bool register_topic()
{
  char TOPIC[128];
  int n = snprintf(TOPIC, sizeof(TOPIC), "sentrifarm/leaf/csv/");
  snprintf(TOPIC + n, sizeof(TOPIC)-n, "%02x%02x%02x%02x%02x%02x", sensorData.mac[0],sensorData.mac[1],sensorData.mac[2],sensorData.mac[3],sensorData.mac[4],sensorData.mac[5]);
  uint16_t topic_id = 0xffff;
  uint8_t idx = 0;
  if (0xffff == (topic_id = MQTTHandler.find_topic_id(TOPIC, idx))) {
    Serial.println(TOPIC);
    Serial.println("Try reg");
    if (!MQTTHandler.register_topic(TOPIC)) {
      Serial.println("Reg error");
      Sentrifarm::deep_sleep_and_reset(FAULT_SLEEP_INTERVAL_MS);
      return false;
    }
    return false;
  }
  // registered
  registered_topic_id  = topic_id;
  return true;
}

// --------------------------------------------------------------------------
ICACHE_FLASH_ATTR
void publish_data()
{
  char buf[192]; // keep it short...
  // For the moment send ASCII
  sensorData.make_mqtt_0(buf, sizeof(buf));
  Serial.println(buf);

  uint8_t flags = FLAG_QOS_1; // 0

  MQTTHandler.publish(flags, registered_topic_id , buf, strlen(buf));
}

ICACHE_FLASH_ATTR
void print_stats()
{
  Serial.print(" state="); Serial.print(state);
  Serial.print(" topic="); Serial.print(registered_topic_id);
  Serial.print(" rx="); Serial.print(metrics.rx_count);
  Serial.print(" tout="); Serial.print(metrics.timeout_count);
  Serial.print(" crc="); Serial.print(metrics.crc_count);
  Serial.print(" dis="); Serial.print(metrics.disconnect);
  Serial.println();
}

int puback_pass_hack = 0;

ICACHE_FLASH_ATTR
void log_mode()
{
  bool crc = false;
  byte buf[128] = { 0 };
  byte rxlen = 0;
  SPI.begin();
  if (radio.ReceiveMessage(buf, sizeof(buf), rxlen, crc))
  {
    Sentrifarm::led4_flash();
    char buf2[256];
    snprintf(buf2, sizeof(buf2), "[RX] %d bytes, crc=%d rssi=%d %02x%02x%02x%02x",
        rxlen, crc, radio.GetLastRssi(), (int)buf[0], (int)buf[1], (int)buf[2], (int)buf[3]);
    Serial.println(buf2);
    Serial.println((char*)buf);
  }
  radio.Standby();
  SPI.end();
}

#if 1
void beacon_tx() {}
#else
ICACHE_FLASH_ATTR
void beacon_tx()
{
#ifdef ESP8266
  // copy the counter into the ESP nvram
  system_rtc_mem_read(64, &beacon_counter, 4);
#endif

  char buf[48];
  snprintf(buf, sizeof(buf), "Beacon %d", (int)beacon_counter);
  beacon_counter ++;
  Serial.println(buf);
  Serial.println(radio.PredictTimeOnAir(strlen(buf)));
  Sentrifarm::led4_on();
  SPI.begin();
  radio.TransmitMessage(buf, strlen(buf));
  radio.Standby();
  SPI.end();
  delay(1000);
  Sentrifarm::led4_off();

#ifdef ESP8266
  // copy the counter into the ESP nvram
  system_rtc_mem_write(64, &beacon_counter, 4);
#endif

  return;
}
#endif

// --------------------------------------------------------------------------
void loop()
{
  if (in_log_mode) { log_mode(); return; }
  if (in_beacon_mode) {
    beacon_tx();
    Sentrifarm::deep_sleep_and_reset(2500); // for some reasy we hit the wdt after about 200 5000ms cycles
    return;
  }

  if (radio.fault()) {
    Serial.println("No radio");
    Sentrifarm::deep_sleep_and_reset(15000);
    return;
  }

  // See if we can receive any radio data
  bool rx_ok = false;
  bool crc = false;
  if (MQTTHandler.TryReceive(crc)) {
    metrics.rx_count ++;
    rx_ok = true;
    Sentrifarm::led4_flash();
  } else if (crc) { metrics.crc_count++; } else { metrics.timeout_count++; }

  if (elapsedStatTime > STATS_INTERVAL_MS) {
    print_stats();
    elapsedStatTime = 0;
  }

  if (elapsedRuntime > RUNTIME_TIMEOUT) {
    Serial.println(F("TOO LONG"));
    delay(100);
    if (puback_pass_hack == 0) {
      if (state != WAIT_PUBACK) { // If QOS is zero then we never get a puback
        Sentrifarm::deep_sleep_and_reset(10000);
      }
    }
    Sentrifarm::deep_sleep_and_reset(ROUTINE_SLEEP_INTERVAL_MS);
    return;
  }

  // Last msg rx'd was corrupted, so ignore it
  if (!rx_ok) { return; }

  // Deal with timeouts and resends
  MQTTHandler.ResetDisconnect();
  if (MQTTHandler.wait_for_response()) {
    if (MQTTHandler.DidDisconnect()) {
      // We got a MQTTSN disconnect for some unexplained reason
      metrics.disconnect ++;
      return;
    }
    // timeout - try again
    if (!MQTTHandler.DidPuback()) {
      return;
    }
  }

  switch (state) {
    case SENT_CONNECT:
      // if we got here, then we processed a CONNACK.
      // register our topic
      if (!register_topic()) {
        // still waiting for it
        return;
      }
      Serial.print(F("HAVE TOPIC")); Serial.println(registered_topic_id);
      elapsedRuntime = 0; // hang around again if we got this far
      // We expect the next message to be a regack
      state = WAIT_REGACK;
      // fall through

    case WAIT_REGACK:
      sensorData.rssi = radio.GetLastRssi();
      sensorData.snr = radio.GetLastSnr();
      publish_data();
      elapsedRuntime = 0; // hang around again if we got this far
      state = WAIT_PUBACK;
      break;

    case WAIT_PUBACK:
      // probably actually a WILL
      puback_pass_hack ++;
      print_stats();
      if (MQTTHandler.DidPuback() || puback_pass_hack > 2) {
        Sentrifarm::deep_sleep_and_reset(ROUTINE_SLEEP_INTERVAL_MS);
      }
      break;

    default:
      break;
  }
}
