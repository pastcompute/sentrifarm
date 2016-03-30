/*
  Copyright (c) 2016 Andrew McDonnell <bugs@andrewmcdonnell.net>

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
#ifndef SENTRIFARM_GLOBALS_H__
#define SENTRIFARM_GLOBALS_H__

namespace sentrifarm {

  static String DEFAULT_PASSWORD = "password";

  static String FALLBACK_SSID = String("sentrifarm__") + String(ESP.getChipId());

  static uint32_t CARRIER_FREQUENCY_HZ = 919000000;

}

// Notes:
// For Serial.println("xxx"):
//   FPSTR makes .text smaller and .data larger, no net change
//   F makes .text larger and .data smaller, no net change
// 
// FPSTR is designed to pool strings in the ESP
//
// but wrapping some strings (not Serial.print) makes the binary larger...?

#endif
