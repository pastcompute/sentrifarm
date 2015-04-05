/*
 * Main entry and timer for sentrifarm ground sensor.
 */

#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"
#include "ds18b20.h"
#include "tcp_push.h"
#include "env.h"
#include <stdio.h>
#include <stdlib.h>

#define DS18B20_GPIO 2
#define SENSOR_POLL_INTERVAL_SECONDS 30

#define DEMO_IPADDRESS "172.30.42.19"
#define DEMO_PORT 7777

static os_timer_t sensor_poll_timer;

/* Handler for poll timer */
static void sensor_poll_timer_cb(void*unused)
{
  struct ds18b20_t results[2];
  int sensors_found=0;
  char buf[6+2*8+7+1+7+2];

	const char *s_ip = env_get("sentri-dst-ip"); 
	const char *s_port = env_get("sentri-dst-port");
  int port = s_port ? atoi(s_port) : DEMO_PORT;

  /*console_printf("Poll sensors...\n");*/

  /* Having multiple DS18B20 on one GPIO saves a gpio but means we need to be able to identify the address after! */
  /* So I think in practice I'll probably have one per GPIO... */

  if (exec_ds18b20(DS18B20_GPIO, results, 2, &sensors_found) && sensors_found > 0) {
    int i;
    struct ds18b20_t* r = results;
    for (i=0; i < sensors_found; i++) {
      snprintf(buf, sizeof(buf), "{\"x\":\"%02x%02x%02x%02x%02x%02x%02x%02x\",\"t\":\"%c%d.%02d\"}",
          r->addr[0], r->addr[1], r->addr[2], r->addr[3], r->addr[4], r->addr[5], r->addr[6], r->addr[7], r->tSign, r->tVal, r->tFract);
      console_printf("%s\n", buf);
      execute_tcp_push(s_ip?s_ip:(const char *)DEMO_IPADDRESS, port, buf);
      r++;
    }
  } else {
    console_printf("Sensor Error\n", buf);
    execute_tcp_push(DEMO_IPADDRESS, port, "{\"x\":\"error\"}");
  }
}


/* This function is called at the end of the init_done() function installed by user_init() */
void sentrifarm_init()
{
  /* Setup a timer to read DS18B20 and send a TCP message */
	const char *s_poll = env_get("sentri-poll");
  os_timer_disarm(&sensor_poll_timer);
  os_timer_setfn(&sensor_poll_timer, (os_timer_func_t *)sensor_poll_timer_cb, NULL);
  os_timer_arm(&sensor_poll_timer, (s_poll ? atoi(s_poll) : SENSOR_POLL_INTERVAL_SECONDS)*1000, 1);

  console_printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  console_printf("~                                         ~\n");
  console_printf("~ HACKADAY                                ~\n");
  console_printf("~         PRIZE                           ~\n");
  console_printf("~              2015                       ~\n");
  console_printf("~                                         ~\n");
  console_printf("~                  ENTRY                  ~\n");
  console_printf("~                       BY                ~\n");
  console_printf("~                         @pastcompute    ~\n");
  console_printf("~                                         ~\n");
  console_printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

}

