#include "Arduino.h"
#include "sf-ioadaptorshield.h"
#include "sf-sensordata.h"

#ifdef TEENSYDUINO
#define Serial Serial1
#endif

namespace Sentrifarm {

  void SensorData::debug_dump() const {
    char buf[256];
    Serial.println(LINE_DOUBLE);

    Serial.print("Software tag ");
    Serial.println(sw_version);
#if defined(ESP8266)
    snprintf(buf, sizeof(buf), "ESP8266: ver=%d boot_mode=%d vcc=%d sdk=%s MAC=%02x:%02x:%02x:%02x:%02x:%02x",
          (int)chipVersion, (int)chipBootMode, (int)chipVcc, sdk, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.println(buf);
#endif

    Serial.print("BOOT COUNT : "); Serial.println(bootCount);

    if (have_date) {
      snprintf((char*)buf, sizeof(buf), "DS1307: 20%02u-%02d-%02u %02u-%02u\n\r",
                  (int)year, (int)month, (int)dayOfMonth, (int)hour, (int)minute);
      Serial.print(buf);
    } else { Serial.println(("DS1307            NOT FOUND")); }

    if (have_radio) {
      snprintf((char*)buf, sizeof(buf), "SX1276 Version  0x%02x\n\r", radio_version);
      Serial.print(buf);
    } else { Serial.println(("Radio           NOT FOUND")); }

    if (have_bmp180) {
      snprintf((char*)buf, sizeof(buf), "Barometer       %4d.%d hPa\n\rAir temperature  %3d.%d degC\n\rAltitude        %4d.%d\n\r",
               (int)floorf(ambient_hpa), fraction(ambient_hpa),
               (int)floorf(ambient_degc), fraction(ambient_degc),
               (int)floorf(altitude_m), fraction(altitude_m));
      Serial.print(buf);
    } else { Serial.println(("Barometer       NOT FOUND")); }

    if (have_pcf8591) {
      int v0 = float(adc_data0) * PCF8591_VREF / 256.F;
      int v1 = float(adc_data1) * PCF8591_VREF / 256.F;
      int v2 = float(adc_data2) * PCF8591_VREF / 256.F;
      int v3 = float(adc_data3) * PCF8591_VREF / 256.F;
      snprintf((char*)buf, sizeof(buf), "PCF8591 Ch#0    %4dmV\n\r", v0); Serial.print(buf);
      snprintf((char*)buf, sizeof(buf), "PCF8591 Ch#1    %4dmV\n\r", v1); Serial.print(buf);
      snprintf((char*)buf, sizeof(buf), "PCF8591 Ch#2    %4dmV\n\r", v2); Serial.print(buf);
      snprintf((char*)buf, sizeof(buf), "PCF8591 Ch#3    %4dmV\n\r", v3); Serial.print(buf);
    } else { Serial.println(("PCF8591       NOT FOUND")); }

    Serial.println(LINE_DOUBLE);
  }

}
