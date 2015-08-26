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
#include <sf-mcu.h>
#include <sf-ioadaptorshield.h>

// Note: for some reason, platformio will fail to add the path to this files include directory to its library if not #include'd by the .ino file
#include <sx1276.h>
#include <mqttsn.h>

#include "sx1276mqttsn.h"
#include "Adafruit_BMP085_U.h"

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

SPISettings spiSettings(1000000, MSBFIRST, SPI_MODE0);

SX1276Radio radio(PIN_SX1276_CS, spiSettings);

MQTTSX1276 MQTTHandler(radio);

// TODO: move a lot of the following to sentrifarm library code and split up

struct NodeSensorData
{
  float air_barometer_hpa;
  float air_temperature_degc;
  float air_altitude_m;
  int radio_version;
};

inline int fraction(float v) { return int((v - floorf(v)) * 10); }

#define F_LINE_DOUBLE F("====================================")

static void report(const struct NodeSensorData& data)
{
  char buf[256];

  Serial.println(F_LINE_DOUBLE);

  snprintf((char*)buf, sizeof(buf), "SX1276 Version  %02x\n", data.radio_version);
  Serial.print(buf);

  snprintf((char*)buf, sizeof(buf), "Barometer       %3d.%d hPa\nAir temperature %3d.%d degC\nAltitude        %3d.%d\n",
           (int)floorf(data.air_barometer_hpa), fraction(data.air_barometer_hpa),
           (int)floorf(data.air_temperature_degc), fraction(data.air_temperature_degc),
           (int)floorf(data.air_altitude_m), fraction(data.air_altitude_m));
  Serial.print(buf);

  Serial.println(F_LINE_DOUBLE);
}

NodeSensorData sensorData;

void read_bmp_once()
{

  // Note: this calls Wire.begin()
  // If experiencing issues with i2c then look at boot ordering
  if(!bmp.begin(BMP085_MODE_STANDARD))
  {
    Serial.println(F("Error detecting BMP-085!"));
  } else {
    sensor_t sensor;
    bmp.getSensor(&sensor);
    Serial.println(F("------------- BMP-085 --------------"));
    Serial.print  (F("Sensor:       ")); Serial.println(sensor.name);
    Serial.print  (F("Driver Ver:   ")); Serial.println(sensor.version);
    Serial.print  (F("Unique ID:    ")); Serial.println(sensor.sensor_id);
    Serial.print  (F("Max Value:    ")); Serial.print(sensor.max_value); Serial.println(F(" hPa"));
    Serial.print  (F("Min Value:    ")); Serial.print(sensor.min_value); Serial.println(F(" hPa"));
    Serial.print  (F("Resolution:   ")); Serial.print(sensor.resolution); Serial.println(F(" hPa"));
    Serial.println(F("------------------------------------"));

    sensors_event_t event;
    bmp.getEvent(&event);
    if (event.pressure) {
      // IDEA: use a 3-axis module to get improved SLP?
      float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
      float pressure = event.pressure;
      float temperature = 0.F;
      bmp.getTemperature(&temperature);
      sensorData.air_altitude_m = bmp.pressureToAltitude(seaLevelPressure, event.pressure);
      sensorData.air_temperature_degc = temperature;
      sensorData.air_barometer_hpa = pressure;
    }
  }
}

void setup()
{
  Sentrifarm::setup_world(F("LEAF Node V0.1"));
  Sentrifarm::setup_shield();
  Sentrifarm::led4_on();
  Sentrifarm::reset_radio();

  Serial.println(F("go..."));
  Sentrifarm::led4_double_short_flash();

  // So, to keep things simple, we read all the sensors now,
  // then use loop() as a state machine processor for publishing the data to the central node

  memset(&sensorData, 0, sizeof(sensorData));

  sensorData.radio_version = radio.ReadVersion();

  read_bmp_once();

  report(sensorData);

  bool ok = MQTTHandler.Begin(&Serial);
}

void loop()
{
  Sentrifarm::deep_sleep_and_reset(10000);
}
