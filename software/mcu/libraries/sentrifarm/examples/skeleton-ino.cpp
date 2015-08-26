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

void setup()
{
  Sentrifarm::setup_world("LEAF Node V0.1");
  Sentrifarm::setup_shield();
  Sentrifarm::led4_on();
  Sentrifarm::reset_radio();

  Serial.println("go...");
  Sentrifarm::led4_double_short_flash();
}

void loop()
{
  Sentrifarm::deep_sleep_and_reset(10000);
}
