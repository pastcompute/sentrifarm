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

#ifndef SENTRIFARM_UTIL_H__
#define SENTRIFARM_UTIL_H__

#define STRINGIFYDEF(x) STRINGIFYDEFS(x)
#define STRINGIFYDEFS(x) #x

#define STR_SF_GIT_VERSION STRINGIFYDEFS(SF_GIT_VERSION)

/// Some platforms (e.g. ESP8266) can't sprintf floats
/// This function gets the fractional part as a number
inline int fraction(float v) { return int((v - floorf(v)) * 10); }

inline byte decToBcd(byte val) { return(val/10*16 + (val%10)); }

inline byte bcdToDec(byte val) { return(val/16*10 + (val%16)); }

#define LINE_DOUBLE "===================================="
#define LINE_SINGLE "------------------------------------"

#endif // SENTRIFARM_UTIL_H__
