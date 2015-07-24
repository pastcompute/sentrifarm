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

extern void ets_intr_lock();
extern void ets_intr_unlock();
extern uint32_t readvdd33(void);

struct envpair {
  char *key, *value;
};

struct envpair defaultenv[] = {
  { "sta-mode",          "dhcp" },
  { "log-level",         "3", },
  { "hostname",          "test1234" },
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
static void dump_info()
{
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
}

static os_timer_t timer1;

static void timer1_cb5(void* unused);
static void timer1_cb4(void* unused);
static void timer1_cb3(void* unused);
static void timer1_cb2(void* unused);
static void timer1_cb(void* unused)
{
  os_intr_lock();
  uint32_t vdd = readvdd33();
  os_intr_unlock();
  uint32_t ticks = system_get_rtc_time();

  console_printf("vcc=%u ticks=%u\n", vdd, ticks);

  wifi_set_phy_mode(PHY_MODE_11B);
  wifi_set_opmode(STATION_MODE);
  wifi_station_dhcpc_start();

  os_timer_disarm(&timer1);
  os_timer_setfn(&timer1, (os_timer_func_t *)timer1_cb2, NULL);
  os_timer_arm(&timer1, 1000, 0);
}

int scounter = 0;

static void timer1_cb2(void* unused)
{
  const char *ssid = "sentri";

  console_printf("try-assoc\n");

	struct station_config sta_conf;
	memset(&sta_conf, 0, sizeof(sta_conf));
	os_strncpy((char*)sta_conf.ssid, ssid, sizeof sta_conf.ssid);

	/* Handle random connection issues: http://41j.com/blog/2015/01/esp8266-wifi-doesnt-connect/ */
	sta_conf.bssid_set = 0;

/*	sta_conf.password[0] = 0x0;*/
/*	if (password != NULL) {*/
/*		os_strncpy((char*)&sta_conf.password, password, 32);*/
/*  }*/

	wifi_station_set_config(&sta_conf);		
  console_printf(".1\n");
	wifi_station_disconnect();
  console_printf(".2\n");
	wifi_station_connect();
  console_printf(".3\n");

  scounter = 0;
  os_timer_disarm(&timer1);
  os_timer_setfn(&timer1, (os_timer_func_t *)timer1_cb3, NULL);
  os_timer_arm(&timer1, 1000, 1);

  console_printf(".4\n");
}

static void timer1_cb3(void* unused)
{
	int state = wifi_station_get_connect_status();
  console_printf("wait state=%d\n", state);
	switch (state) {
	case STATION_CONNECT_FAIL:
		console_printf("STATION_CONNECT_FAIL\n");
    break;
	case STATION_NO_AP_FOUND:
		console_printf("STATION_NO_AP_FOUND\n");
    break;
	case STATION_GOT_IP:
		console_printf("STATION_GOT_IP\n");
    break;
  case STATION_WRONG_PASSWORD:
		console_printf("STATION_WRONG_PASS\n");
    break;
  default:
    console_printf(".");
	  scounter++;
	  if (scounter > 20) {
		  console_printf("Timeout, still trying\n");
      break;
	  }
    return;
  }
  console_printf(".5\n");
	os_timer_disarm(&timer1);
  os_timer_setfn(&timer1, (os_timer_func_t *)timer1_cb4, NULL);
  os_timer_arm(&timer1, 10000, 0);
	console_lock(0);
	return;
}

static void timer1_cb4(void* unused)
{
  console_printf("4.\n");
  wifi_station_disconnect();
	os_timer_disarm(&timer1);
  os_timer_setfn(&timer1, (os_timer_func_t *)timer1_cb5, NULL);
  os_timer_arm(&timer1, 2000, 0);
}

static void timer1_cb5(void* unused)
{
  console_printf("5.\n");
  system_deep_sleep(3*1000*1000);
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
  }
  dump_info();

  wifi_station_set_auto_connect(0);
  wifi_set_opmode(STATION_MODE);
  wifi_station_disconnect();
  system_deep_sleep_set_option(4);  // 4 - disable RF after wake-up. but how do we turn it back on 2 - no RF cal 1 - RF Cal (large current) 0 depends on byte 108 (?)

  os_timer_disarm(&timer1);
  os_timer_setfn(&timer1, (os_timer_func_t *)timer1_cb, NULL);
  os_timer_arm(&timer1, 1000, 0);
  console_lock(0);
}

/* ------------------------------------------------------------------------- */
static void main_init_done(void)
{
  console_printf("2.\n");
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
}

/* ------------------------------------------------------------------------- */
void user_init()
{
  set_log_level(3);
  // system_set_os_print(0);  // seems to turn off dbg() even when log-level = 3
  uart_init(0, 115200);
  uart_init_io();

  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
  gpio_output_set(0, BIT2, BIT2, 0);
  gpio_output_set(0, BIT0, BIT0, 0);

  dump_info();

  /* To minimise stack we just query 16 bytes at a time */
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

  env_init(CONFIG_ENV_OFFSET, CONFIG_ENV_LEN);
  console_init(32);

  console_printf("1.\n");

  system_init_done_cb(main_init_done);
}

