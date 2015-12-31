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
#include <Wire.h>

// Note this is backwards to the default (doh)
#define PIN_SDA          5
#define PIN_SCL          4

#define PCF8591_VREF 2490

#define PCF8591_I2C_ADDR (0x90 >> 1)

void scan_i2c_bus()
{
  Serial.println(F("Scanning i2c bus."));
  for (byte addr=3; addr < 127; addr++) {
    char buf[32];
    Wire.beginTransmission(addr);
    int error = Wire.endTransmission();
    if (error == 0) {
      snprintf(buf, sizeof(buf), "i2c device at %02x", (int)addr);
      Serial.println(buf);
    }
  }
}

// --------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  Serial.println("ADC Test");
  Wire.pins(PIN_SDA, PIN_SCL);
  Wire.begin();
  scan_i2c_bus();
  delay(500);
  Serial.println("Run");
}

static bool powerup = true;

// --------------------------------------------------------------------------
void loop()
{
  byte adcValue = 0;
  byte dummy = 0;
  int er = 0;
  char buf[256];

#ifdef GOOD_SINGLE
  for (int ch=0; ch <4; ch++) {
    Wire.beginTransmission(PCF8591_I2C_ADDR);
//    Wire.write(ch); // | 0x40);
    Wire.write(ch | 0x40);
    Wire.write(0);
    if ((er=Wire.endTransmission()) != 0) {
      Serial.print("i2c fail 1 reading channel"); Serial.print(ch); Serial.print("code");Serial.println(er);
      goto faux;
    }
    Wire.requestFrom(PCF8591_I2C_ADDR, 2);
    if (Wire.available()) {
      dummy = Wire.read(); // dummy, which will be 0x80 on power on
    } else {
      Serial.println("i2c fail 2");
      goto faux;
    }
    if (Wire.available()) {
      adcValue = Wire.read();
      //snprintf(buf, sizeof(buf), "Read ADC#%d PRIOR %02x", ch, dummy);
      //Serial.println(buf);
      int v0 = float(adcValue) * PCF8591_VREF / 256.F;
      //snprintf(buf, sizeof(buf), "Read ADC#%d %02x %4d", ch, adcValue, v0);
      //Serial.println(buf);
      snprintf(buf, sizeof(buf), "%02x ", adcValue);
      Serial.print(buf);
    } else {
      Serial.println("i2c fail 3");
    }
    delay(100);
  }
  Serial.println();
#else
  // averaging test
  for (byte ch=0; ch <4; ch++) {
    Wire.beginTransmission(PCF8591_I2C_ADDR);
    Wire.write(ch | 0x40);   // Enable DAC when using internal oscillator
    Wire.write(0);
    if (Wire.endTransmission() != 0) {
      Serial.print("i2c fail 1 reading channel"); Serial.print(ch); Serial.print("code");Serial.println(er);
      goto faux;
    }
    // Read each value 5 times and average/discard, to try and account for the fact
    // that randomly there will be junk
    byte value[5];
    for (byte r=0; r < 5; r++) {
      Wire.requestFrom(PCF8591_I2C_ADDR, 2);
      if (!Wire.available()) { Serial.println("i2c fail 2"); goto faux; }
      Wire.read();
      if (!Wire.available()) { Serial.println("i2c fail 3"); goto faux; }
      value[r] = Wire.read();
    }
    float mean = 0;
    for (byte r=0; r < 5; r++) {
      mean += value[r];
      snprintf(buf, sizeof(buf), "%02x ", value[r]);
      Serial.print(buf);
    }
    Serial.print(" : ");
    mean /= 5.F;
    // Scan for data that is way off the mean and discard it
    byte dist[5];
    float meandist = 0;
    for (byte r=0; r < 5; r++) {
      meandist += (dist[r] = abs(float(value[r]) - mean));
      snprintf(buf, sizeof(buf), "%02x ", dist[r]);
      Serial.print(buf);
    }
    Serial.print(" : ");
    meandist /= 5;
    // re-average
    if (meandist > 0) {
      mean = 0;
      byte nvalid = 0;
      for (byte r=0; r < 5; r++) {
        if (dist[r] < meandist) {
          Serial.print("+");
          mean += value[r]; nvalid ++;
        } else {
          Serial.print("-");
        }
      }
      if (nvalid > 0) {
        mean /= nvalid;
      }
    }
    Serial.print(" : ");
    adcValue = mean;
    int v0 = float(adcValue) * PCF8591_VREF / 256.F;
    snprintf(buf, sizeof(buf), "%02x %4d", adcValue, v0);

    delay(100);
    Serial.println();
  }
#endif
faux:
  delay(1000);
}
