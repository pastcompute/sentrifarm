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

#ifndef SENTRIFARM_DS1307_H__
#define SENTRIFARM_DS1307_H__

#include "sf-sensordata.h"
#include "sf-util.h"

namespace Sentrifarm {

  void read_datetime_once(SensorData& sensorData)
  {
    int error;
    Wire.beginTransmission(D1307_I2C_ADDR);
    Wire.write(0);
    if ((error=Wire.endTransmission()) != 0) {
      Serial.print(F("DS1307 FAIL E1:")); Serial.println(error);
      sensorData.have_date = false;
      return;
    }
    Wire.requestFrom(D1307_I2C_ADDR, 8);
    sensorData.second = bcdToDec(Wire.read() & 0x7f);
    sensorData.minute = bcdToDec(Wire.read());
    sensorData.hour = bcdToDec(Wire.read() & 0x3f);
    sensorData.dayOfWeek = bcdToDec(Wire.read());
    sensorData.dayOfMonth = bcdToDec(Wire.read());
    sensorData.month = bcdToDec(Wire.read());
    sensorData.year = bcdToDec(Wire.read());
    Wire.read(); // control register
    if ((error=Wire.endTransmission()) != 0) {
      Serial.print(F("DS1307 FAIL E2:")); Serial.println(error);
      sensorData.have_date = false;
      return;
    }
    sensorData.have_date = true;
  }

  // Read a value in the nvram
  // addr : offset from 0x8 , valid for 0..0x37 ie 0x8 .. 0x3f)
  byte read_nvram_byte(int addr)
  {
    byte value = 0xff;
    int error;
    Wire.beginTransmission(D1307_I2C_ADDR);
    Wire.write(addr + 0x8);
    if ((error=Wire.endTransmission()) != 0) {
      Serial.print(F("DS1370 NVRAM FAIL #0 code ")); Serial.println(error);
    }
    Wire.requestFrom(D1307_I2C_ADDR, 1);
    value = Wire.read();
    if ((error=Wire.endTransmission()) != 0) {
      Serial.print(F("DS1370 NVRAM FAIL #1 code ")); Serial.println(error);
    }
    return value;
  }

  // Save a value in the nvram
  // addr : offset from 0x8 , valid for 0..0x37 ie 0x8 .. 0x3f)
  void save_nvram_byte(int addr, byte value)
  {
    int error;
    Wire.beginTransmission(D1307_I2C_ADDR);
    Wire.write(addr + 0x8);
    Wire.write(value);
    if ((error=Wire.endTransmission()) != 0) {
      Serial.print(F("DS1370 NVRAM FAIL #2 code ")); Serial.println(error);
    }
  }

  uint8_t bin2bcd(uint8_t val) { return val + 6 * (val / 10); }

  extern void read_datetime_once(SensorData& sensorData);

  }

#endif // SENTRIFARM_DS1307_H__
