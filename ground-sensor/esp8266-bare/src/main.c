#define USE_US_TIMER
#include "osapi.h"
#include "driver/uart.h"
#include "gpio.h"
#include "user_interface.h"
#include "helpers.h"

int log_level = 2; // required by lwip

const char* fr_request_hostname(void) { // required by lwip
  return "esp8266";
}

static os_timer_t scan_completed_timer;

static volatile int scan_finished = 0;

static void scan_completed_timer_cb(void*unused)
{
  os_timer_disarm(&scan_completed_timer);
  if (!scan_finished) {
    ets_uart_printf("wireless scan not completed?\n");
  }
  // This requires hardware mod: http://tim.jagenberg.info/2015/01/18/low-power-esp8266/
  system_deep_sleep(55000000);    
}

static void wireless_scan_done_cb(void *arg, STATUS status)
{
	scaninfo *c = arg; 
	struct bss_info *inf; 
  ets_uart_printf("wireless %d\n", status);
	if (!c->pbss) {
    scan_finished = 1;
		return;
  }
	STAILQ_FOREACH(inf, c->pbss, next) {
		ets_uart_printf("BSSID %02x:%02x:%02x:%02x:%02x:%02x channel %02d rssi %02d auth %-12s %s\n", 
				MAC2STR(inf->bssid),
				inf->channel, 
				inf->rssi, 
				id_to_encryption_mode(inf->authmode),
				inf->ssid
			);
		inf = (struct bss_info *) &inf->next;
	}
  scan_finished = 1;
}

void user_init()
{
  system_timer_reinit();
	uart_init(0, 115200);
	uart_init_io();

	ets_uart_printf("\n\n\nBare Minimum ESP8266 Firmware\n");
	ets_uart_printf("Powered by Antares " CONFIG_VERSION_STRING "\n");	
	ets_uart_printf("(c) Andrew McDonnell (@pastcompute) 2015 <bugs@andrewmcdonnell.net>\n");	
	ets_uart_printf("Antares is (c) Andrew 'Necromant' Andrianov 2014 <andrew@ncrmnt.org>\n");	
	ets_uart_printf("This is free software (where possible), published under the terms of GPLv2\n\n");	
	system_print_meminfo();

  // system_set_os_print(0);

  scan_finished = 0;
  wifi_set_opmode(STATION_MODE);
	wifi_station_scan(NULL, &wireless_scan_done_cb);
	wifi_station_scan(NULL, &wireless_scan_done_cb);

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	gpio_output_set(0, BIT2, BIT2, 0);
	gpio_output_set(0, BIT0, BIT0, 0);

  os_timer_disarm(&scan_completed_timer);
  os_timer_setfn(&scan_completed_timer,  (os_timer_func_t *)scan_completed_timer_cb, NULL);
  os_timer_arm(&scan_completed_timer, 5000, 0); // give it 5 seconds then deep sleep
}
