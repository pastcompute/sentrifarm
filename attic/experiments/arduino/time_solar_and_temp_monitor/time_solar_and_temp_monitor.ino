#include <Wire.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <stdarg.h>

#define I2C_ADDR_RTC 0x68
#define PIN_BUZZER 11
#define PIN_LED 13
#define PIN_LED_GREEN 9
#define PIN_LED_BLUE 10
#define PIN_BUTTON 8
#define PIN_1WIRE 5

static OneWire ds(PIN_1WIRE);
static DallasTemperature sensors(&ds);

static DeviceAddress ds18b20;
static bool ds_good = false;

static byte bcd2dec(byte val)
{
  return ( (val/16*10) + (val%16) );
}

static byte dec2bcd(byte val)
{
  return ( (val % 10) + (val/10*16) );
}

static void p(char *fmt, ...)
{
  char buf[256];
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end (args);
  Serial.print(buf);
}

static void Tone(int buzzer, int Freq, int duration)
{ 
  int uSdelay = 1000000 / (Freq * 2);
  unsigned long ending = millis() + duration;
  while(millis() < ending){
    digitalWrite(buzzer, HIGH);
    delayMicroseconds(uSdelay);
    digitalWrite(buzzer, LOW);
    delayMicroseconds(uSdelay);
  }
}

void setup()
{
  Serial.begin(9600);  // start serial for output
  digitalWrite(PIN_LED, HIGH);
  digitalWrite(PIN_LED_GREEN, HIGH);
  digitalWrite(PIN_LED_BLUE, HIGH);
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  Wire.begin();        // join i2c bus (address optional for master)

  Serial.println(F("boot"));
  Tone(PIN_BUZZER, 990, 120); 
  Wire.beginTransmission(I2C_ADDR_RTC);
  Wire.write(7);
  Wire.write(0x0);
  Wire.endTransmission();
  delay(5000);
  digitalWrite(PIN_LED_BLUE, HIGH);
  Tone(PIN_BUZZER, 990, 120);

  delay(5000);
  Serial.println(F("boot2"));
  digitalWrite(PIN_LED, LOW);


//  Serial.println(F("Init osc"));
//  Wire.beginTransmission(I2C_ADDR_RTC);
//  Wire.write(0);
//  Wire.write(0);
//  Wire.endTransmission();  
//  delay(2000);
//
  if (0) {
    Serial.println(F("Init clock"));
    Wire.beginTransmission(I2C_ADDR_RTC);
    Wire.write(0);
    Wire.write(0);
    Wire.write(0x33);
    Wire.write(0x22);
    Wire.write(0x07);
    Wire.write(0x04);
    Wire.write(0x04);
    Wire.write(0x15);
    Wire.endTransmission();
  }

  ds_good = false;
  byte addr[8];
  if (ds.search(addr)) {
    p("1-Wire %.02x:%.02x:%.02x:%.02x:%.02x:%.02x:%.02x:%.02x",
        addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]); 
    if (addr[7] != OneWire::crc8(addr,7)) {
      Tone(PIN_BUZZER, 390, 1700);
      Serial.println(F("1-wire CRC error"));
    } else {
      Tone(PIN_BUZZER, 790, 700);
      ds_good = true;
      for (int i=0; i<8; i++) { ds18b20[i] = addr[i]; }
      sensors.begin();
      sensors.setResolution(ds18b20, 10);
    }
  } else {
   Serial.println(F("1-wire Not found"));
  }

  if (false) {
    Serial.println(F("Init sqw"));
    Wire.beginTransmission(I2C_ADDR_RTC);
    Wire.write(7);
    Wire.write(0x13);
    Wire.endTransmission();
  }
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_BLUE, LOW);
}

void readDsRtcTime(byte *second, 
                    byte *minute, 
                    byte *hour, 
                    byte *dayOfWeek, 
                    byte *dayOfMonth, 
                    byte *month, 
                    byte *year)
{
  Wire.beginTransmission(I2C_ADDR_RTC);
  Wire.write(0); // set DS3232 register pointer to 00h
  Wire.endTransmission();  
  Wire.requestFrom(I2C_ADDR_RTC, 7); 
  *second     = bcd2dec(Wire.read() & 0x7f);
  *minute     = bcd2dec(Wire.read());
  *hour       = bcd2dec(Wire.read() & 0x3f);  
  *dayOfWeek  = bcd2dec(Wire.read());
  *dayOfMonth = bcd2dec(Wire.read());
  *month      = bcd2dec(Wire.read());
  *year       = bcd2dec(Wire.read());
}

void dump()
{
  for (int n=0; n <= 0x3f; ) {
    Wire.beginTransmission(I2C_ADDR_RTC);
    Wire.write(n); // set DS3232 register pointer to 00h
    Wire.endTransmission();  
    Wire.requestFrom(I2C_ADDR_RTC, 1);
    if (Wire.available()) {
      byte x = Wire.read();
      p("%.02x ", x);
    } else { Serial.print("-- "); }
    n++;
    if (n%16==0) Serial.println();
  }
  Serial.print(F("\n"));
}

void loop()
{
  //dump();

  digitalWrite(PIN_LED, HIGH);
  byte s,m,h,w,d,i,y;
  readDsRtcTime(&s, &i, &h, &w, &d, &m, &y);
  sensors.requestTemperatures();
  float degC = sensors.getTempCByIndex(0);
  // millivolts rel 5V - use long to ensure enough bits for multiply without float
//  long v1 = analogRead(0); delay(125);
//  long v2 = analogRead(1); delay(125);
  long v3 = analogRead(2); delay(125);
  long v4 = analogRead(3); delay(125);
  long v5 = analogRead(4); delay(125);
  long v6 = analogRead(5); delay(125);

//  v1 = (v1 * 5000L) / 1023L;
//  v2 = (v2 * 5000L) / 1023L;
  v3 = (v3 * 5000L) / 1023L;
  v4 = (v4 * 5000L) / 1023L;
  v5 = (v5 * 5000L) / 1023L;
  v6 = (v6 * 5000L) / 1023L;

  p("20%02d-%02d-%02d %02d:%02d:%02d un %04ldmV V3 %04ldmV Vs %04ldmV Vd %04ldmV ",
        y, m, d, h, i, s, v3, v4, v5, v6);
  Serial.println(degC);
  digitalWrite(PIN_LED, LOW);

  //if (digitalRead(PIN_BUTTON) == 0) { Serial.println("press");  }
  //delay(1000); return;

  int count = 300; // seconds
  while (--count) {
    delay(1000);
    if (digitalRead(PIN_BUTTON) == 0) { Serial.println("press"); break; }
  }
  
}
