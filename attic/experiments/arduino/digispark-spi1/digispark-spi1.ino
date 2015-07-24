/*
 * First attempt to use a digispark to drive an SPI device. Specifcially, the SX1276 lora module.
 * They are still in the mail so this is just a test to get a handle on flash usage
 * SPI related code forked from digispark RFM12b example.
 *
 * So far, so good: uses 558 / 6012 bytes. But then we have no libraries (which killed us last time)
 *
 * Lets add in the digispark half duplex soft-serial serial port, bearing in mind on Pin3 means we
 * lose the ability to be a USB device. Alternative is to pretend to be a keyboard, but that
 * gets annoying. Why cant we be a USB serial port over USB? Although both those methods use 1 more pin.
 *
 * Upshot: soft serial debug adds 2500 bytes. So use sparingly...
 *
 * The CRC function adds ~48 bytes inline per call.
 */

#define SERIAL_DEBUG_ENABLED 1

#if SERIAL_DEBUG_ENABLED
#include <TinyPinChange.h>
#include <SoftSerial.h>
#endif

#define PIN_SPI_MOSI PB0
#define PIN_SPI_MISO PB1
#define PIN_SPI_SCK PB2
#define PIN_HALF_DUPLEX_SERIAL PB3
#define PIN_SPI_CS0 PB5

#define CS_ENABLE() PORTB &= ~_BV(PIN_SPI_CS0)
#define CS_DISABLE() PORTB |= _BV(PIN_SPI_CS0)

#if SERIAL_DEBUG_ENABLED
SoftSerial DebugSerial(PIN_HALF_DUPLEX_SERIAL, PIN_HALF_DUPLEX_SERIAL, true);
#endif

/* This function pulses the SPI clock USICLK which is hard-wired onto PB2 for 8 bits */
static void spi_run_clock () {
  USICR = _BV(USIWM0) | _BV(USITC);
  USICR = _BV(USIWM0) | _BV(USITC) | _BV(USICLK);
  USICR = _BV(USIWM0) | _BV(USITC);
  USICR = _BV(USIWM0) | _BV(USITC) | _BV(USICLK);
  USICR = _BV(USIWM0) | _BV(USITC);
  USICR = _BV(USIWM0) | _BV(USITC) | _BV(USICLK);
  USICR = _BV(USIWM0) | _BV(USITC);
  USICR = _BV(USIWM0) | _BV(USITC) | _BV(USICLK);
  USICR = _BV(USIWM0) | _BV(USITC);
  USICR = _BV(USIWM0) | _BV(USITC) | _BV(USICLK);
  USICR = _BV(USIWM0) | _BV(USITC);
  USICR = _BV(USIWM0) | _BV(USITC) | _BV(USICLK);
  USICR = _BV(USIWM0) | _BV(USITC);
  USICR = _BV(USIWM0) | _BV(USITC) | _BV(USICLK);
  USICR = _BV(USIWM0) | _BV(USITC);
  USICR = _BV(USIWM0) | _BV(USITC) | _BV(USICLK);
}

/* Write two bytes to the SPI port */
static void spi_write(uint8_t highbyte, uint8_t lowbyte)
{
  CS_ENABLE();
  USIDR = highbyte;
  spi_run_clock();
  USIDR = lowbyte;
  spi_run_clock();
  CS_DISABLE();
}

static void spi_read_write(byte command, byte& result)
{
  CS_ENABLE();
  USIDR = command;
  spi_run_clock();
  result = USIDR;
  CS_DISABLE();
}

static __inline__ uint16_t _crc16_update(uint16_t __crc, uint8_t __data)
{
	uint8_t __tmp;
	uint16_t __ret;

	__asm__ __volatile__ (
		"eor %A0,%2" "\n\t"
		"mov %1,%A0" "\n\t"
		"swap %1" "\n\t"
		"eor %1,%A0" "\n\t"
		"mov __tmp_reg__,%1" "\n\t"
		"lsr %1" "\n\t"
		"lsr %1" "\n\t"
		"eor %1,__tmp_reg__" "\n\t"
		"mov __tmp_reg__,%1" "\n\t"
		"lsr %1" "\n\t"
		"eor %1,__tmp_reg__" "\n\t"
		"andi %1,0x07" "\n\t"
		"mov __tmp_reg__,%A0" "\n\t"
		"mov %A0,%B0" "\n\t"
		"lsr %1" "\n\t"
		"ror __tmp_reg__" "\n\t"
		"ror %1" "\n\t"
  		"mov %B0,__tmp_reg__" "\n\t"
  		"eor %A0,%1" "\n\t"
  		"lsr __tmp_reg__" "\n\t"
  		"ror %1" "\n\t"
  		"eor %B0,__tmp_reg__" "\n\t"
  		"eor %A0,%1"
  		: "=r" (__ret), "=d" (__tmp)
  		: "r" (__data), "0" (__crc)
  		: "r0"
  	);
  	return __ret;
}

void setup() {
  
#if SERIAL_DEBUG_ENABLED
  DebugSerial.begin(38400);
  DebugSerial.txMode();
  DebugSerial.println(F("\nDebug enabled"));
  DebugSerial.rxMode();
#endif

  /* The digispark only has PortB. Here we allocate:
  
  P0 --> MOSI (USI Din)
  P1 --> MISO (USI Dout)  (note: also the on-board LED)
  P2 --> SCK  (USI clock)
  P5 --> CS   Chip select for the SX1276
  
  Note that P2 is hard-wired onto USICLK for the ATtiny85
 
  Thus, we need to make P1, P2 and P5 output, and set P5 high initially (CS is active LOW).
  
  */
  DDRB = _BV(PIN_SPI_MISO) | _BV(PIN_SPI_SCK) | _BV(PIN_SPI_CS0);

  /* initially deselected */
  PORTB = _BV(PIN_SPI_CS0);
  
  /* put USI into 3-wire software clock mode */
  USICR = _BV(USIWM0);

  /* Send a pretend SPI command */
  int x = 0, y = 0;
  x = _crc16_update(0x1234, 0x56);
  y = _crc16_update(0x1234, 0x78);
  spi_write(0x0f, x & 0xff);
  
}

void loop() {
  /* Send a pretend SPI command */
  spi_write(0x5a, 0xa5);
  
  /* Send a command that returns a response */
  byte byte1;
  spi_read_write(0x00, byte1);

  delay(5000);
}
