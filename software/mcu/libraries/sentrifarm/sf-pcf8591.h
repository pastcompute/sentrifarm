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
    byte adcValue0;
    byte adcValue1;
    byte adcValue2;
    byte adcValue3;

    Wire.beginTransmission(PCF8591_I2C_ADDR);
    Wire.write(0x4); // Auto increment - request all 4 ADC channels
    Wire.write(0x0); // Placeholder - unused DAC
    if (Wire.endTransmission() != 0) {
      sensorData.have_pcf8591 = false;
      return;
    }
    Wire.requestFrom(PCF8591_I2C_ADDR, 5);
    Wire.read(); // dummy
    adcValue0 = Wire.read();
    adcValue1 = Wire.read();
    adcValue2 = Wire.read();
    adcValue3 = Wire.read();
    if (Wire.endTransmission() == 0) {
      sensorData.adc_data0 = adcValue0;
      sensorData.adc_data1 = adcValue1;
      sensorData.adc_data2 = adcValue2;
      sensorData.adc_data3 = adcValue3;
      sensorData.have_pcf8591 = true;
    } else {
      sensorData.have_pcf8591 = false;
    }
  }

}


#endif // SENTRIFARM_PCF8591_H__
