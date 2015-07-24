#define USE_US_TIMER
#include "osapi.h"
#include "driver/uart.h"
#include "gpio.h"
#include "user_interface.h"
#include "helpers.h"

static os_timer_t scan_start_timer;
static os_timer_t scan_completed_timer;

static volatile int scan_finished = 0;
static volatile int scan_counter = 0;

static void scan_completed_timer_cb0(void*unused)
{
  if (scan_finished) {
    ets_uart_printf("Scan complete.\n");
    os_timer_disarm(&scan_completed_timer);
    // This requires hardware mod: http://tim.jagenberg.info/2015/01/18/low-power-esp8266/
    system_deep_sleep(55000000);
    return;
  }

  scan_counter ++;
  if (scan_counter < 25) {
    ets_uart_printf("Waiting for scan to complete (%d)\n", scan_counter);
    return;
  }

  ets_uart_printf("wireless scan not completed?\n");
  os_timer_disarm(&scan_completed_timer);
  system_deep_sleep(55000000);
}

static void wireless_scan_done_cb(void *arg, STATUS status)
{
	scaninfo *c = arg; 
	struct bss_info *inf; 
  ets_uart_printf("Scan: wireless: %d %d\n", status, (int)c->pbss);
	if (!c->pbss) {
    scan_finished = 1;
    ets_uart_printf("Scan: wireless: fail\n");
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
  ets_uart_printf("Scan: wireless: finish\n");
}

static void scan_start_timer_cb(void*unused)
{
  // wifi_promiscuous_enable(1);
  wifi_set_opmode(STATION_MODE);
  wifi_station_set_auto_connect(FALSE);
	if (wifi_station_scan(NULL, &wireless_scan_done_cb)) {
  	ets_uart_printf("Scanning...\n");	
  } else {
  	ets_uart_printf("Unable to scan!\n");	
  }
}

static void my_init_done(void)
{
  /* Start a wifi scan in five seconds */
	ets_uart_printf("Ready in 5...\n");	
  scan_counter = 0;
  os_timer_disarm(&scan_start_timer);
  os_timer_setfn(&scan_start_timer,  (os_timer_func_t *)scan_start_timer_cb, NULL);
  os_timer_arm(&scan_start_timer, 5000, 0);
}

void user_init()
{
  system_timer_reinit();
	uart_init(0, 115200);
	uart_init_io();

	wdt_feed();
	system_set_os_print(1); 

	ets_uart_printf("\n\n\nBare Minimum ESP8266 Firmware\n");
	ets_uart_printf("Powered by Antares " CONFIG_VERSION_STRING "\n");	
	ets_uart_printf("(c) Andrew McDonnell (@pastcompute) 2015 <bugs@andrewmcdonnell.net>\n");	
	ets_uart_printf("Antares is (c) Andrew 'Necromant' Andrianov 2014 <andrew@ncrmnt.org>\n");	
	ets_uart_printf("This is free software (where possible), published under the terms of GPLv2\n\n");	
	system_print_meminfo();

  unsigned char macaddr[6];
  wifi_get_macaddr(STATION_IF, macaddr);
	ets_uart_printf("sdk=%s chipid=0x%x mac=" MACSTR " heap=%d\n",
      system_get_sdk_version(), system_get_chip_id(), MAC2STR(macaddr), system_get_free_heap_size());

/*	struct ip_info info;*/
/*	wifi_get_ip_info(STATION_IF, &info);*/
/*  info.ip.addr = ipaddr_addr("10.0.111.111");*/
/*  info.netmask.addr = ipaddr_addr("255.255.255.0");*/
/*  wifi_set_ip_info(STATION_IF, &info);*/
/*  scan_finished = 0; */

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	gpio_output_set(0, BIT2, BIT2, 0);
	gpio_output_set(0, BIT0, BIT0, 0);

  system_init_done_cb(my_init_done);
 	ets_uart_printf("Setup.\n");	
}

