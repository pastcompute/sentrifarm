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

#ifndef SENTRIFARM_PCF8591_H__
#define SENTRIFARM_PCF8591_H__

#include "sf-sensordata.h"

namespace Sentrifarm {

  // --------------------------------------------------------------------------
  void read_pcf8591_once(SensorData& sensorData)
  {
    byte adcValue = 0;
    sensorData.have_pcf8591 = false;

    for (byte ch=0; ch <4; ch++) {
      Wire.beginTransmission(PCF8591_I2C_ADDR);
      Wire.write(ch | 0x40);   // Enable DAC when using internal oscillator
      Wire.write(0);
      if (Wire.endTransmission() != 0) {
        return;
      }
      // Read each value 5 times and average/discard, to try and account for the fact
      // that randomly there will be junk
      byte value[5];
      for (byte r=0; r < 5; r++) {
        Wire.requestFrom(PCF8591_I2C_ADDR, 2);
        if (!Wire.available()) { goto done; }
        Wire.read();
        if (!Wire.available()) { goto done; }
        value[r] = Wire.read();
      }
      float mean = 0;
      for (byte r=0; r < 5; r++) {
        mean += value[r];
      }
      mean /= 5.F;
      // Scan for data that is way off the mean and discard it
      byte dist[5];
      float meandist = 0;
      for (byte r=0; r < 5; r++) {
        meandist += (dist[r] = abs(float(value[r]) - mean));
      }
      meandist /= 5;
      // re-average
      if (meandist > 0) {
        mean = 0;
        byte nvalid = 0;
        for (byte r=0; r < 5; r++) {
          if (dist[r] < meandist) { mean += value[r]; nvalid ++; }
        }
        if (nvalid > 0) {
          mean /= nvalid;
        }
      }
      adcValue = mean;
      switch (ch) {
      case 0: sensorData.adc_data0 = adcValue; break;
      case 1: sensorData.adc_data1 = adcValue; break;
      case 2: sensorData.adc_data2 = adcValue; break;
      case 3: sensorData.adc_data3 = adcValue; break;
      }
      delay(100);
    }
    sensorData.have_pcf8591 = true;
done:
    Wire.beginTransmission(PCF8591_I2C_ADDR);
    Wire.write(0);
    Wire.endTransmission();
  }
}


#endif // SENTRIFARM_PCF8591_H__
