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

#define PIN_DHT 2

#include "Arduino.h"

#include <DHT.h>

DHT HumiditySensor(PIN_DHT,DHT11);

// --------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  Serial.println("DHT11 Test");
  delay(500);
  HumiditySensor.begin();
}


// --------------------------------------------------------------------------
void loop()
{
  Serial.println("DHT11 Loop");
  bool hh = HumiditySensor.read();
  if (hh) {
    float humidity;
    humidity = HumiditySensor.readHumidity();
    Serial.print("H/T ");
    Serial.print(humidity);
    Serial.print(",");
    Serial.print(HumiditySensor.readTemperature());
    Serial.println();
  } else {
    digitalWrite(PIN_DHT, HIGH);
    Serial.println("H/T ERROR");
  }
  delay(5000);
}
