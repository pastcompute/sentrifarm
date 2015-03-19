#include <TinyPinChange.h>
#include <SoftSerial.h>
#include "DHT.h"

// Serial: Standard RS232: 
// 47k between gnd and common join
// 1N4148 anode to common join, cathode to TXD : DB9:pin3, DB25:pin2
// common join to RXD: DB9:pin2, DB25:pin3
// common join to 4k7
// 4k7 to Pin (0,2,etc)
// Half duplex comms!
// USe picocom --b 38400
#define DEBUG_TX_RX_PIN 0
SoftSerial DbgSerial( DEBUG_TX_RX_PIN, DEBUG_TX_RX_PIN, true);

DHT dht;

void setup() {
  // put your setup code here, to run once:
  DbgSerial.begin(38400);
  DbgSerial.txMode();
  dht.setup(2); // pin2
  digitalWrite(1, HIGH);
  DbgSerial.println(F("\nReady."));
  delay(1500);
  digitalWrite(1, LOW);
}

void loop() {
  delay(dht.getMinimumSamplingPeriod());
  float temp = dht.getTemperature();
  float humid = dht.getHumidity();
  DbgSerial.print(temp); DbgSerial.print(" degrees; ");
  DbgSerial.print(humid); DbgSerial.print(" humidity.");
  DbgSerial.println("");
  digitalWrite(1, HIGH);
  delay(200);
  digitalWrite(1, LOW); 
  delay(1000);
}
