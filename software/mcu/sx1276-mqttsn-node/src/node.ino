#include "Arduino.h"
#include <SPI.h>
#include <elapsedMillis.h>
#include "sx1276.h"
#include "mqttsn.h"
#include "mqttsn-messages.h"
#include "sx1276mqttsn.h"
#if defined(ESP8266)
#include <ets_sys.h>
#else
#define ICACHE_FLASH_ATTR
#endif

#include "Adafruit_BMP085_U.h"

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

MQTTSX1276 MQTTHandler(radio);

bool started_ok = false;

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

#if defined(ESP8266)
extern "C" uint16_t readvdd33(void);

// Dodgy bros binary data for reducing transmission bandwidth
struct __attribute__ ((__packed__)) my_data_t {
  byte bootversion;
  byte bootmode;
  uint16_t vcc;
  uint32_t cycles;
};

void pack_demo_data(struct my_data_t& data)
{
  data.bootversion = ESP.getBootVersion();
  data.bootmode = ESP.getBootMode();
  data.vcc = readvdd33(),
  data.cycles = ESP.getCycleCount();
}
#endif

char tmp_buf[256];

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

bool bmp_good = false;

byte bmp_buf[66] = { 0, };
byte bmp_len = 0;

void poll_bmp()
{
  sensors_event_t event;
  bmp.getEvent(&event);
  if (event.pressure) {
    // How does this work with ESP8266 soft float?
    // IDEA: use the 3-axis thing to get better SLP?
    float pressure = event.pressure;
    float temperature;
    bmp.getTemperature(&temperature);
    float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
    float altitude = bmp.pressureToAltitude(seaLevelPressure, event.pressure);

    bmp_len = snprintf((char*)bmp_buf, sizeof(bmp_buf), "HPA=%d.%d DEGC=%d.%d ALT=%d.%d",
                       (int)floorf(pressure), int((pressure - floorf(pressure)) * 10),
                       (int)(temperature), int((temperature - floorf(temperature)) * 10),
                       (int)(altitude), int((altitude - floorf(altitude)) * 10)
                   );

  }
}

#define PCF8591 (0x90 >> 1)

byte adc_buf[66] = { 0, };
byte adc_len = 0;

byte adcValue0;
byte adcValue1;
byte adcValue2;
byte adcValue3;

void poll_adc()
{
  Wire.begin();
  Wire.beginTransmission(PCF8591);
  Wire.write(0x4); // Auto increment - request all 4 ADC channels
  Wire.endTransmission();
  Wire.requestFrom(PCF8591, 5);
  Wire.read(); // dummy
  adcValue0 = Wire.read();
  adcValue1 = Wire.read();
  adcValue2 = Wire.read();
  adcValue3 = Wire.read();

  adc_len = snprintf((char*)adc_buf, sizeof(adc_buf), "%02x %02x %02x %02x", adcValue0, adcValue1, adcValue2, adcValue3);
}

void setup()
{
  delay(1000); // let last of ESP8266 junk get past
  Serial.begin(115200);
  Serial.println();

  Serial.print(F("\n\n\nSentrifarm : sx1276 mqttsn node: "));
#if defined(TEENSYDUINO)
  Serial.println(F("TEENSY-LC"));
#elif defined(ESP8266)
  // ADC_MODE(ADC_VCC); // Unlike suggested by the doco, doesnt work
  Serial.println(F("ESP8266 ESP-201"));
#endif

#if defined(ESP8266)
  my_data_t demo;
  pack_demo_data(demo);
  snprintf(tmp_buf, sizeof(tmp_buf), "%02x %02x %u %ld %s",
                     demo.bootversion, demo.bootmode,
                     (unsigned)demo.vcc, demo.cycles,
                     ESP.getSdkVersion());
  Serial.println(tmp_buf);
#endif

  pinMode(PIN_LED4,        OUTPUT);
  pinMode(PIN_SX1276_RST,  OUTPUT);
  pinMode(PIN_SX1276_CS,   OUTPUT);

  // Power on the LED (active low2)
  digitalWrite(PIN_LED4, LOW);

#if defined(TEENSYDUINO)
  SPI.setSCK(PIN_SX1276_SCK);
#endif

#if defined(ESP8266)
  // Annoyingly this is using deprecated to set the static default because the adafruit library calls Wire.begin()
  // what a clusterf**k
  Wire.pins(5, 4); // Note this is backwards to the default (doh)
#endif

  // Luckily I went with the teensy default for I2c (18,19) on my shield


  if(!bmp.begin(BMP085_MODE_STANDARD))
  {
    Serial.println("Error detecting BMP-085!");
  } else {
    sensor_t sensor;
    bmp.getSensor(&sensor);
    Serial.println("------------- BMP-085 --------------");
    Serial.print  ("Sensor:       "); Serial.println(sensor.name);
    Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
    Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
    Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" hPa");
    Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" hPa");
    Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" hPa");
    Serial.println("------------------------------------");
    poll_bmp();
    Serial.println((char*)bmp_buf);
    bmp_good = true;
  }

  poll_adc();
  Serial.println((char*)adc_buf);


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

  started_ok = MQTTHandler.Begin(&Serial);

  if (started_ok) {
    // FIXME: do this on powerup but not deep sleep wake
    byte hello_payload[6] = { 0, 0, 0, 0, 0, 0 };
    SPI.begin();
    for (byte x=0; x < 5; x++) {
      hello_payload[1] = x;
      hello_payload[5] = MQTTHandler.xorvbuf(hello_payload, 5);

      radio.TransmitMessage(hello_payload, 6);
      double_short();
      delay(500);
    }
    radio.Standby();
    SPI.end();
    delay(500);
  }
}

bool got_message = false;

int rx_count = 0;
int crc_count = 0;
int timeout_count = 0;

elapsedMillis timeElapsed;
elapsedMillis regackTimedown;
elapsedMillis connectTimedown;

bool need_connect = true;
bool sent_connect = false;
bool need_rx = true;
bool all_done_nearly = false;
bool wait_regack = false;

int regack_ttl = 10;

void loop() {

  if (bmp_good) {
    poll_bmp();
  }
  if (!started_ok) {
    digitalWrite(PIN_LED4, LOW);
    delay(500);
    digitalWrite(PIN_LED4, HIGH);
    delay(500);
    return;
  }

  if (need_connect && !sent_connect) {
    MQTTHandler.connect(0, 30, "sfnode1"); // keep alive in seconds
    sent_connect = true;
    all_done_nearly = false;
    connectTimedown = 0;
  }
  bool crc;
  bool ok = false;
  if (need_rx) {
    if (MQTTHandler.TryReceive(crc)) {
      rx_count++;
      ok = true;
      digitalWrite(PIN_LED4, LOW);
      delay(200);
      digitalWrite(PIN_LED4, HIGH);
    } else if (crc) { crc_count++; } else { timeout_count++; }
  }
  if (timeElapsed > 6000)  {
    Serial.print("Received message count: "); Serial.print(rx_count);
    Serial.print(" RX count: "); Serial.print(rx_count);
    Serial.print(" Timeout count: "); Serial.print(timeout_count);
    Serial.print(" CRC count: "); Serial.print(crc_count);
    Serial.print(" State: ");
    Serial.print(need_connect);
    Serial.print(sent_connect);
    Serial.print(need_rx);
    Serial.print(wait_regack);
    Serial.print(all_done_nearly);
    Serial.println();
    timeElapsed = 0;

    if (all_done_nearly) {
      // work around bug where response get very delayed
      go_to_sleep(10000);
      return;
    }
  }
  if (!ok) {
    return;
  }
  if (wait_regack) {
    Serial.println("was waiting for regack");
    wait_regack = false;
  }
  MQTTHandler.ResetDisconnect(); // dodgy thing
  if (MQTTHandler.wait_for_response()) {
    // if connect / register / needs response and havent got it yet
    // try again
    // the method will call disconnect if retry failed
    if (MQTTHandler.DidDisconnect()) {
      Serial.println("RESTART ON TIMEOUT DISCONNECT");
      go_to_sleep(1000);
      return;
    }
    return;
  }
  Serial.println("Got some kind of response");
  need_connect = false;
  // What happened when we got the _wrong_ response?

  const char TOPIC[] = "sentrifarm/node/beacon";
  uint16_t topic_id = 0xffff;
  uint8_t idx = 0;
  if (0xffff == (topic_id = MQTTHandler.find_topic_id(TOPIC, idx))) {
    Serial.println("Try register");
    if (!MQTTHandler.register_topic(TOPIC)) {
      Serial.println("Register error");
      go_to_sleep(1000);
      return;
    }
    wait_regack = true;
    regackTimedown = 0;
    // needs response, so loop around
    return;
  }

  // we dont get back again until after the following publish gets a resopnse
  if (all_done_nearly) {
    // Deep sleep
    Serial.println("Deep Sleep");
    go_to_sleep(10000);
  }
  Serial.println("Try publish");
  byte pub_buf[66];
  byte pub_len = 0;
  // ASCII would be nice but we only have 66 bytes to play with

  if (bmp_len > 0) {
    MQTTHandler.publish(FLAG_QOS_0, topic_id, bmp_buf, bmp_len);
  } else {
    MQTTHandler.publish(FLAG_QOS_0, topic_id, "BMP_BAD", 7);
  }

  MQTTHandler.publish(FLAG_QOS_0, topic_id, adc_buf, adc_len);

#if defined(ESP8266)
  my_data_t demo;
  pack_demo_data(demo);
  pub_len = snprintf((char*)pub_buf, sizeof(pub_buf), "%02x %02x %u %lu %s",
                     demo.bootversion, demo.bootmode,
                     (unsigned)demo.vcc, demo.cycles,
                     ESP.getSdkVersion());
#else
  pub_len = snprintf((char*)pub_buf, sizeof(pub_buf), "Hello,World");
#endif

  MQTTHandler.publish(FLAG_QOS_0, topic_id, pub_buf, pub_len);
  all_done_nearly = true;
}
