#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "gpio.h"
#include "driver/uart.h" 
#include "microrl.h"
#include "console.h"
#include "helpers.h"
#include "flash_layout.h"
#include <generic/macros.h>
#include <lwip/netif.h>
#include <lwip/app/dhcpserver.h>

#include "env.h"
#include "iwconnect.h"

struct envpair {
  char *key, *value;
};

struct envpair defaultenv[] = {
  { "sta-mode",          "dhcp" },
  { "log-level",         "3", },
  { "sentri-ssid",       "sentri" },
  { "sentri-password",   "" },
  { "hostname",          "groundstation" },
  { "bootdelay",         "3" },
};

void request_default_environment(void)
{
  int i;
  for (i=0; i<ARRAY_SIZE(defaultenv); i++)
    env_insert(defaultenv[i].key, defaultenv[i].value);
}

const char* fr_request_hostname(void)
{
  return env_get("hostname");
}

/* ------------------------------------------------------------------------- */
static void scan_done_cb(void *arg, STATUS status)
{
  scaninfo *c = arg;
  struct bss_info *inf;
  if (c->pbss) {
    STAILQ_FOREACH(inf, c->pbss, next) {
      console_printf("BSSID %02x:%02x:%02x:%02x:%02x:%02x channel %02d rssi %02d auth %-12s %s\n",
          MAC2STR(inf->bssid),
          inf->channel,
          inf->rssi,
          id_to_encryption_mode(inf->authmode),
          inf->ssid
        );
      inf = (struct bss_info *) &inf->next;
    }
  } else {
    console_printf("ERROR: Scan invalid pbss. STATUS=%d opmode=%d\n", status, (int)wifi_get_opmode());

    // We are probably in the dead bug, so do a restart
    system_restart();
  }

  unsigned char macaddr[6] = { 0,0,0,0,0,0};
  unsigned char macaddr2[6] = { 0,0,0,0,0,0};
  wifi_get_macaddr(SOFTAP_IF, macaddr);
  wifi_get_macaddr(STATION_IF, macaddr);
  console_printf("sdk=%s chipid=0x%x ap.mac=" MACSTR " sta.mac=" MACSTR " heap=%d tick=%u rtc.tick=%u\n",
    system_get_sdk_version(), system_get_chip_id(), MAC2STR(macaddr), MAC2STR(macaddr2),
    system_get_free_heap_size(), system_get_time(), system_get_rtc_time());

  console_lock(0);
  extern void sentrifarm_init();
  sentrifarm_init();
}

/* ------------------------------------------------------------------------- */
static void main_init_done(void)
{
  if (wifi_get_opmode() == SOFTAP_MODE)
  {
    console_printf("ERROR: Can't scan, while in softap mode\n");
  } else {
    if (wifi_station_scan(NULL, &scan_done_cb)) {
      console_lock(1);
      return;
    }
    console_printf("ERROR: Call to wifi_station_scan() failed\n");
  }
  extern void sentrifarm_init();
  sentrifarm_init();
}

/* ------------------------------------------------------------------------- */
void user_init()
{
  uart_init(0, 115200);
  uart_init_io();

  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
  gpio_output_set(0, BIT2, BIT2, 0);
  gpio_output_set(0, BIT0, BIT0, 0);

  env_init(CONFIG_ENV_OFFSET, CONFIG_ENV_LEN);
  system_print_meminfo();
  console_printf("\nFlash layout:\n"
           "Firmware size is     %d KiB (0x%x)\n"
           "Filesystem starts at %d KiB (0x%x)\n",
           fr_get_firmware_size()/1024, fr_get_firmware_size(),
           fr_fs_flash_offset()/1024,   fr_fs_flash_offset());

  uint8 f = system_get_cpu_freq();
  console_printf("CPU frequency=%dMHz\n", (int)f);

  unsigned char macaddr[6] = { 0,0,0,0,0,0};
  unsigned char macaddr2[6] = { 0,0,0,0,0,0};
  wifi_get_macaddr(SOFTAP_IF, macaddr);
  wifi_get_macaddr(STATION_IF, macaddr);
  console_printf("sdk=%s chipid=0x%x ap.mac=" MACSTR " sta.mac=" MACSTR " heap=%d tick=%u rtc.tick=%u\n",
    system_get_sdk_version(), system_get_chip_id(), MAC2STR(macaddr), MAC2STR(macaddr2),
    system_get_free_heap_size(), system_get_time(), system_get_rtc_time());

  console_printf("WIFI opmode=%s sleep=%s auto=%s status=%s phy=%s\n",
      id_to_wireless_mode(wifi_get_opmode()), id_to_wifi_sleep_type(wifi_get_sleep_type()),
      wifi_station_get_auto_connect()?"yes":"no", id_to_sta_state(wifi_station_get_connect_status()),
      id_to_phy_mode(wifi_get_phy_mode())); 

  /* To minimise stack we just query 16 bytes at a type */
  const int bytes = 768;
  const int words_per_row = 8;
  const int bytes_per_row = words_per_row * sizeof(uint32_t);
  uint32_t rtc_values[words_per_row];
  for (int row=0; row < bytes/bytes_per_row; row++) {
    system_rtc_mem_read(row * words_per_row, rtc_values, bytes_per_row);
    /* print address on start of line */
    console_printf("%04x ", row*bytes_per_row);
    for (int w=0; w < words_per_row; w++) {
      console_printf("%08x ", rtc_values[w]);
    }
    const char *chars = (const char*)&rtc_values[0];
    for (int c=0; c < bytes_per_row; c++) {
      char ch = chars[c];
      console_printf("%c", ch < 32 || ch > 95 ? '.' : ch);
    }
    console_printf("\n");
  }

  console_init(32);

  // system_set_os_print(0);  // seems to turn off dbg() even when log-level = 3

  const char* loglevel = env_get("log-level");
  if (loglevel) { set_log_level(atoi(loglevel)); }
  
  system_init_done_cb(main_init_done);
}

