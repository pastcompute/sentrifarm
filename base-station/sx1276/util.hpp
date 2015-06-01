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
#ifndef UTIL_HPP__
#define UTIL_HPP__

#include <string>
#include <string.h>

namespace util {

inline std::string buf2str(const void *data, unsigned len)
{
  std::string result;
  const char *p = (const char *)data;
  for (unsigned i=0; i < len; i++) {
    char c = *p++;
    result += iscntrl(c) ? '.' : c;
  }
  return result;
}

// Requires : #define _XOPEN_SOURCE 600
inline std::string safe_perror(int code, const char *prefix)
{
  char buf[96] = "";
  strerror_r(code, buf, sizeof(buf));
  if (strlen(buf)==0) { snprintf(buf, sizeof(buf), "Error code %d", code); }
  return (prefix ? (std::string(prefix) + ": ") : std::string()) + std::string(buf);
}

};

#endif // UTIL_HPP__
