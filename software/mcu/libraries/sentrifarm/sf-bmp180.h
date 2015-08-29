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

#ifndef SENTRIFARM_BMP180_H__
#define SENTRIFARM_BMP180_H__

#include "sf-sensordata.h"
#include <Adafruit_BMP085_U.h>

namespace Sentrifarm {

  // --------------------------------------------------------------------------
  void read_bmp_once(SensorData& sensorData, Adafruit_BMP085_Unified& bmp)
  {
    // Note: this calls Wire.begin()
    // If experiencing issues with i2c then look at boot ordering
    if(!bmp.begin(BMP085_MODE_STANDARD))
    {
      Serial.println(F("Error detecting BMP-085!"));
      sensorData.have_bmp180 = false;
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
        sensorData.ambient_hpa = pressure;
        sensorData.ambient_degc = temperature;
        sensorData.altitude_m = bmp.pressureToAltitude(seaLevelPressure, event.pressure);
        sensorData.have_bmp180 = true;
      } else {
        Serial.println(F("Error reading BMP-085!"));
        sensorData.have_bmp180 = false;
      }
    }
  }

}

#endif // SENTRIFARM_BMP180_H__
