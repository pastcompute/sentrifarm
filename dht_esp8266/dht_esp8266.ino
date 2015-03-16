#include <TinyPinChange.h>
#include <SoftSerial.h>
#include "DHT.h"

// We get one or the other of these in an ATtiny85 8kb...
// Compiler / architecture must be a bit inefficient, given what aVic20 could do
// Base is ~ 2kb
#define USE_DEBUG_SERIAL 0   // Ads 3.5kb total 5300+ bytes
#define ENABLE_ESP8266 1 // Adds 2kb for just plain old access pint, nodisacrds responses : total 4204 bytes

#if USE_DEBUG_SERIAL
SoftSerial DbgSerial( 0, 0, true); // true for connection via resistor & diode to DB9/25 12V RS232

#define DEBUG_START() DbgSerial.begin(38400); DbgSerial.txMode();
#define DEBUG_PRINTLN(x) DbgSerial.println(x)
#define DEBUG_PRINT(x) DbgSerial.print(x)

#else

#define DEBUG_START() {}
#define DEBUG_PRINTLN(x) {}
#define DEBUG_PRINT(x) {}

#endif




// Diskspark pinouts
// 0: debug serial
// 1: onboard LED
// 2: DHT-11
// 3: TX -> ESP8266
// 4: RX ,_ ESP8266

// Serial: Standard RS232: 
// 47k between gnd and common join
// 1N4148 anode to common join, cathode to TXD : DB9:pin3, DB25:pin2
// common join to RXD: DB9:pin2, DB25:pin3
// common join to 4k7
// 4k7 to Pin (0,2,etc)
// Half duplex comms!
// Use picocom --b 38400

DHT dht;

#if ENABLE_ESP8266

// Connect the ESP TX to pin4 (RX) and  RX to pin4 (TX)
// Use a 3v3 source to power the ESP! I used a USB-serial dongle
// Dont flash the digispark with the ESP powered up
// Seems to work best if you flash with ESP unplugged
// Then unplug both, plug in the ESP wait a bit then plug in the digispark
//
// Notably, the digispark wont program with a LE + resistor wired onto pin6?
// Also, beware of ground loops:
// To flash, remove all connections to ESP or USB dongle

SoftSerial esp8266( 3, 4);
//char esp8266Response[32];

void esp8266Send(fstr_t *command, const int timeout=1000)
{
  esp8266.print(command); // send the read character to the esp8266
  const long int check = millis() + timeout;
  int n=0;
  do {
    while(esp8266.available()) {
      char c = esp8266.read();
      n++;
//      if (discardResponse) continue;
//      if (n<31) esp8266Response[n++] = c;
      DEBUG_PRINT(c);
    }
  } while (check > millis());
  DEBUG_PRINT(n);
  
//  esp8266Response[n] = 0;
//  if (debug) DbgSerial.println(command);
//  if (debug) DbgSerial.println(response);
}

// Setup to ignore responses...
#define ESP8266_START() esp8266.begin(9600);
#define ESP8266_SEND(command ...) esp8266Send(command)

#else

#define ESP8266_SEND(command ...)

#endif

void setup() {
  ESP8266_START();
  DEBUG_START();
  dht.setup(2);

  // Initial test: become an AP so we can scan for it
  DEBUG_PRINTLN(F("\r\nSetup..."));
  digitalWrite(1, HIGH); delay(1500); digitalWrite(1, LOW);
  ESP8266_SEND(F("AT+RST\r\n"),2000); // reset module
  delay(500); digitalWrite(1, HIGH);
  ESP8266_SEND(F("AT+CWMODE=2\r\n")); // configure as access point
  // Sometimes hanging here: why?
  delay(1500); digitalWrite(1, LOW);
  //ESP8266_SEND(F("AT+CIFSR")); // get ip address
  //ESP8266_SEND(F("AT+CIPMUX=1")); // configure for multiple connections
  //ESP8266_SEND(F("AT+CIPSERVER=1,80")); // turn on server on port 80
  DEBUG_PRINTLN(F("\nReady."));
  delay(300);
}

void loop() {
  delay(dht.getMinimumSamplingPeriod());
  digitalWrite(1, HIGH);
  float temp = dht.getTemperature();
  float humid = dht.getHumidity();
//  DbgSerial.print(" tick(s)="); DbgSerial.print(millis()/1000); DbgSerial.print(" "); 
//  DbgSerial.print(temp); DbgSerial.print(" degrees; ");
//  DbgSerial.print(humid); DbgSerial.print(" humidity.");
  // DEBUG_PRINT(millis()); DEBUG_PRINT(F(" ")); // ~40 bytes
  DEBUG_PRINT(temp); DEBUG_PRINT(F(" "));  
  // DEBUG_PRINT(humid); DEBUG_PRINTLN(); // 40 bytes
  delay(250);  digitalWrite(1, LOW); delay(250); digitalWrite(1, HIGH); delay(250);  digitalWrite(1, LOW); 
//  ESP8266_SEND(F("AT\r\n"));
  delay(1000);
}
