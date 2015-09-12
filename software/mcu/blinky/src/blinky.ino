#include "Arduino.h"

#ifdef ESP82666
  #define LED_PIN 2
#endif

#ifndef LED_PIN
  #define LED_PIN 13
#endif

#ifdef TEENSYDUINO
// Physical pins 2+3 (or labelled 0,1) NOT 8,10 as per the Eagle diagram?
#define Serial Serial1
#endif

void setup()
{
  pinMode(LED_PIN, OUTPUT);     // set pin as output
  Serial.begin(115200);
  Serial.println("Hello, World from PlatformIO");
}
int i=0;
void loop()
{
  Serial.print("Loop ");
  Serial.println(i);
  i++;
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
}
