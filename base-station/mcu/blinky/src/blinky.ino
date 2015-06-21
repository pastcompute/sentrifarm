#include "Arduino.h"

#ifndef LED_PIN
  // Most Arduino boards already have a LED attached to pin 13 on the board itself
  #define LED_PIN 13
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
  digitalWrite(14, HIGH);
  delay(500);
  digitalWrite(14, LOW);
  delay(500);
}
