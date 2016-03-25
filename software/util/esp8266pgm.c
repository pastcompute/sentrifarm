#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

void report_state(int fd)
{
  int pins=0;
  ioctl(fd, TIOCMGET, &pins);
  printf("Current State: DTR(RST)=%s RTS(PGM)=%s\n", (pins & TIOCM_DTR) ? "0V" : "3V3", (pins & TIOCM_RTS) ? "0V" : "3V3");
}

void toggle_dtr(int fd, bool force_low)
{
  int pins=0;
  ioctl(fd, TIOCMGET, &pins);

  bool bit = (pins & TIOCM_DTR);

  pins = pins & ~TIOCM_DTR | (force_low ? 0 : (!bit ? TIOCM_DTR : 0));
  ioctl(fd, TIOCMSET, &pins);
}

void toggle_rts(int fd, bool force_low)
{
  int pins=0;
  ioctl(fd, TIOCMGET, &pins);

  bool bit = (pins & TIOCM_RTS);

  pins = pins & ~TIOCM_RTS | (force_low ? 0 : (!bit ? TIOCM_RTS : 0));
  ioctl(fd, TIOCMSET, &pins);
}

int main(int argc, char *argv[])
{
  const char *d = "/dev/ttyUSB0";
  const char *p = getenv("ESP_PORT");
  if (p) { d = p; }

  // DTR is 3v3 UNTIL open() is called, then it falls to 0V until toggled because the kernel open() handler in serial_core.c is stupid
  int fd = open(d, O_RDONLY | O_NOCTTY | O_NDELAY);

  // Very quickly, re-assert DTR and RTS and hope that the glitch doesnt reset the ESP8266
  // Because open() for whatever reason is inverting the state
  int pins=0;
  ioctl(fd, TIOCMGET, &pins);
  pins = pins & (~(TIOCM_RTS | TIOCM_DTR));
  ioctl(fd, TIOCMSET, &pins);
  report_state(fd);

  sleep(2);


  // Assert DTR (hold reset on the ESP8266 to 0V)
  toggle_dtr(fd, true);  // Force 3v3
  toggle_dtr(fd, false); // Set to 0V

  // Assert RTS (hold PGM on the ESP8266 to 0V)
  toggle_rts(fd, true);  // Force 3v3
  toggle_rts(fd, false); // Set to 0V

  report_state(fd);
  sleep(1);

  // Deassert DTR (invert reset on the ESP8266 to 3V3)
  toggle_dtr(fd, true);  // Force 3v3
  sleep(1);

  // Deassert RTS (revert PGM on the ESP8266 to 3V3)
  toggle_rts(fd, true);  // Force 3v3
  report_state(fd);
  printf("Done.\n");
  return 0;
}

