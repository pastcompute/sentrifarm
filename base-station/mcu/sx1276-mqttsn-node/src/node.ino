#include "Arduino.h"
#include <SPI.h>
#include <elapsedMillis.h>
#include "sx1276.h"

#include <TaskManager.h>

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
#include <c_types.h>
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
#define ICACHE_FLASH_ATTR
#else
#error "Unsupported configuration"
#endif

SPISettings spiSettings(1000000, MSBFIRST, SPI_MODE0); // double check

SX1276Radio radio(PIN_SX1276_CS, spiSettings);

bool started_ok = false;

ICACHE_FLASH_ATTR
void setup()
{
  delay(1000); // let last of ESP8266 junk get past
  Serial.begin(115200);
  Serial.println();

  Serial.print(F("\n\n\nSentrifarm : sx1276 mqttsn node: "));
#if defined(TEENSYDUINO)
  Serial.println(F("TEENSY-LC"));
#elif defined(ESP8266)
  Serial.println(F("ESP8266 ESP-201"));
#endif

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

  // Reset the sx1276 module
  digitalWrite(PIN_SX1276_RST, LOW);
  delay(10); // spec states to pull low 100us then wait at least 5 ms
  digitalWrite(PIN_SX1276_RST, HIGH);
  delay(50);

  // init SPI and then program the chip to LoRa mode
  SPI.begin();
  Serial.print(F("SX1276: version=")); Serial.println(radio.ReadVersion());
  if (!radio.Begin()) {
    Serial.println(F("SX1276 init error"));
    // TODO: flash the LED
  } else {
    radio.SetCarrier(919000000);
    uint32_t carrier_hz = 0;
    radio.ReadCarrier(carrier_hz);
    Serial.print(F("Carrier: ")); Serial.println(carrier_hz);
    started_ok = true;
  }
  SPI.end();
  delay(500);
}

void go_to_sleep(int ms)
{
#if defined(ESP8266)
  ESP.deepSleep(ms * 1000, WAKE_RF_DISABLED); // long enough to see current fall on the USB power meter
  delay(500); // Note, deep sleep actually takes a bit to activate, so we want to make sure loop doesnt do anything...
#else
  delay(ms);
#endif
}

void double_short()
{
  digitalWrite(PIN_LED4, HIGH);
  delay(250);
  digitalWrite(PIN_LED4, LOW);
  delay(250);
  digitalWrite(PIN_LED4, HIGH);
  delay(250);
  digitalWrite(PIN_LED4, LOW);
  delay(250);
  digitalWrite(PIN_LED4, HIGH);
}

bool got_message = false;

int rx_count = 0;
int crc_count = 0;
int timeout_count = 0;

elapsedMillis timeElapsed;

void loop() {
  if (!started_ok) {
    digitalWrite(PIN_LED4, LOW);
    delay(500);
    digitalWrite(PIN_LED4, HIGH);
    delay(500);
    return;
  }

}
