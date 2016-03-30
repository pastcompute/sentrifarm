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
#ifndef SENTRIFARM_MCU_H__
#define SENTRIFARM_MCU_H__

// Pin allocations for the ESP-07 + inAir9b prototype board

#define PIN_SX1276_RST   0
#define PIN_SX1276_CS    2
#define PIN_SX1276_MISO 12
#define PIN_SX1276_MOSI 13
#define PIN_SX1276_SCK  14

#define SWAP_SERIAL_BEGIN() { yield();  delay(500); Serial.swap(); } // give time for serial data to flush through
#define SWAP_SERIAL_END() { Serial.swap(); yield();  delay(500); }

#endif
