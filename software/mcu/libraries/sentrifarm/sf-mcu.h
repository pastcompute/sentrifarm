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

#ifndef SENTRIFARM_MCU_H__
#define SENTRIFARM_MCU_H__

#if defined(ESP8266)

#include <c_types.h>
#include <ets_sys.h>
#include <Esp.h> // deep sleep

#elif defined(TEENSYDUINO)

#else

#error "Unsupported hardware configuration"

#endif

#ifndef ICACHE_FLASH_ATTR
#define ICACHE_FLASH_ATTR
#endif

namespace Sentrifarm {

  void setup_world(const String& description);

  void setup_shield();

  void reset_radio();

  void deep_sleep_and_reset(int ms);

  void led4_on();
  void led4_off();
  void led4_double_short_flash();

}

#endif // SENTRIFARM_MCU_H__
