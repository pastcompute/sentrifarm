#include "Arduino.h"
extern "C" {
#include "user_interface.h"
}

namespace sentrifarm {

bool CheckForUserResetWifiOnBoot()
{
  // if   the reboot was due to _power_on_ (not reset button)
  // then 
  //      first sleep for 2 seconds 
  //      check if there is a short over GPIO (the pin normally used for programming)
  //      if the short persists over 3 seconds then reset the wifi configuration
  //
  // i.e.
  //      the user needs to power cycle, and then short _after_ 2 seconds...
  //      and continue to hold it for another three seconds
  //      if they are monitoring the serial port the have up to 5 seconds in theory
  //
  // we can help them by turning the serial activity LED onto solid blue
  //
  Serial.print(F("SF: Reset reason: ")); Serial.println(ESP.getResetReason());
  if (ESP.getResetInfoPtr()->reason == REASON_DEFAULT_RST) {
    Serial.print(F("SF: Short GPIO0 now for 5 seconds, to reset WIFI data"));
    // TODO: turn on LED to indicate that button needs to be pressed..
    delay(2000);
    int npress = 0;
    while (digitalRead(0) == 0 && npress < 7) {
      Serial.println(F("."));
      npress ++;
      delay(300);
    }
    if (npress >= 7) {
      Serial.println(F("USER RESET of WIFI DATA. Reboot..."));
      return false;
    }
  }
  return true;
}

}
