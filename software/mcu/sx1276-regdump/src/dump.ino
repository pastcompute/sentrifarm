#include "Arduino.h"
#include <SPI.h>

// Supports the following configurations:
//
// (1) ESP8266, using sentrifarm-shield-adaptor
//     GPIO12 = MISO (HSPI)
//     GPIO13 = MOSI (HSPI)
//     GPIO14 = SCK (HSPI)
//     GPIO15 = CS
//     GPIO0  = RST
//     GPIO2  = LED4 (inverted)
//
// (2) TEENSY LC, using sentrifarm-shield-adaptor
//     MOSI,11
//     MISO,12
//     SCK,14
//     CS,10
//     RST,21
//     LED4,5

#if defined(ESP8266)
#include <Esp.h> // deep sleep
#define PIN_LED4         4
#define PIN_SX1276_RST   0
#define PIN_SX1276_CS   15
#define PIN_SX1276_MISO 12
#define PIN_SX1276_MOSI 13
#define PIN_SX1276_SCK  14
#elif defined(TEENSYDUINO)
#define PIN_LED4         5
#define PIN_SX1276_RST  21
#define PIN_SX1276_CS   10
#define PIN_SX1276_MISO 12
#define PIN_SX1276_MOSI 11
#define PIN_SX1276_SCK  14
#else
#error "Unsupported configuration"
#endif

SPISettings spiSettings(1000000, MSBFIRST, SPI_MODE0); // double check

void setup()
{
  Serial.begin(115200);
  Serial.println("Sentrifarm : sx1276 register dump");

  pinMode(PIN_LED4,        OUTPUT);
  pinMode(PIN_SX1276_RST,  OUTPUT);
  pinMode(PIN_SX1276_CS,   OUTPUT);

  // Power on the LED (active low2)
  digitalWrite(PIN_LED4, LOW);

#if defined(TEENSYDUINO)
  SPI.setSCK(PIN_SX1276_SCK);
#endif

  digitalWrite(PIN_SX1276_CS, HIGH);
  digitalWrite(PIN_SX1276_RST, HIGH);
  digitalWrite(PIN_SX1276_MISO, HIGH);
  digitalWrite(PIN_SX1276_MOSI, HIGH);
  digitalWrite(PIN_SX1276_SCK,  HIGH);

}


void loop() {

  digitalWrite(PIN_LED4, LOW);
	SPI.begin();

  // Reset the sx1276
  digitalWrite(PIN_SX1276_RST, LOW);
  delay(10); // spec states to pull low 100us then wait at least 5 ms
  digitalWrite(PIN_SX1276_RST, HIGH);
  delay(50);

  char buf[4];
  for (byte r=0; r <= 0x70; r++) {
    if (r % 8 == 0) {
      snprintf(buf, sizeof(buf), "%02x ", r);
      Serial.print(buf);
    }
    SPI.beginTransaction(spiSettings);
    digitalWrite(PIN_SX1276_CS, LOW);
    SPI.transfer(r);
    byte value = SPI.transfer(0);
    digitalWrite(PIN_SX1276_CS, HIGH);
    SPI.endTransaction();
    snprintf(buf, sizeof(buf), "%02x ", value);
    Serial.print(buf);
    if (r % 8 == 7) { Serial.println(); }
  }
  Serial.println();

  delay(100);

	SPI.end();
  digitalWrite(PIN_LED4, HIGH);

#if defined(ESP8266)
  Serial.println("Request deep sleep");
	// ESP.deepSleep(8000000, WAKE_RF_DEFAULT); // long enough to see current fall on the USB power meter
	ESP.deepSleep(8000000, WAKE_RF_DISABLED); // long enough to see current fall on the USB power meter
	delay(500);
  Serial.println("Requested deep sleep"); // we should not ever see this
#else
	// FIXME
	delay(7000);
#endif
}

