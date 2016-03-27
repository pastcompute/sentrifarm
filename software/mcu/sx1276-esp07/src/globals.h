#ifndef SENTRIFARM_GLOBALS_H__
#define SENTRIFARM_GLOBALS_H__

namespace sentrifarm {

namespace constants {

  static String FALLBACK_SSID = "sentrifarm__" + String(ESP.getChipId());

}

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
