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
  { "sentri-ssid",       CONFIG_ENV_DEFAULT_STATION_AUTO_SSID },
  { "sentri-password",   CONFIG_ENV_DEFAULT_STATION_AUTO_PASSWORD },
  { "hostname",          CONFIG_ENV_DEFAULT_HOSTNAME },
  { "bootdelay",         "1" },
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

static void main_init_done(void)
{
  extern void sentrifarm_init();
  sentrifarm_init();
}

void user_init()
{
  uart_init(0, 115200);
  uart_init_io();
  env_init(CONFIG_ENV_OFFSET, CONFIG_ENV_LEN);
  system_print_meminfo();
  system_set_os_print(0);
  console_printf("\n Flash layout:\n"
           "Firmware size is     %d KiB (0x%x)\n"
           "Filesystem starts at %d KiB (0x%x)\n",
           fr_get_firmware_size()/1024, fr_get_firmware_size(),
           fr_fs_flash_offset()/1024,   fr_fs_flash_offset());
  wifi_set_opmode(id_from_wireless_mode("STA"));

  /* TODO: work out how ti add our own config */
  console_init(32);

  const char* loglevel = env_get("log-level");
  if (loglevel) { set_log_level(atoi(loglevel)); }
  
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
  gpio_output_set(0, BIT2, BIT2, 0);
  gpio_output_set(0, BIT0, BIT0, 0);

  system_init_done_cb(main_init_done);
}

