/* ------------------------------------------------------------------------- */
/*
 * Main entry and timer for sentrifarm ground sensor.
 * Just because I'll do it later, most stuff is in this one big file for the moment...
 *
 * Preliminary Architectural Design
 * --------------------------------
 *
 * Because I have seen strange resets / crashes when a timer takes too long (i.e. >= 1 seconds)
 * everything is currently done by chained timers / state machine.
 * Presumably the underlying Espressif SDK O/S is actually not truly multi tasking, or something.
 *
 *
 * Flow of Events
 * --------------
 *
 * Boot / wake from deep sleep --> sentrifarm_init()
 * --> If GPIOx (bypass switch) is connected to GND, then skip sentrifram processing and become a normal frankenstein
 * --> If GPIOx (debug mode switch) is connected to GND, then turn on debug LED
 * --> Ensure wifi is off (should be boot disabled)
 * --> Sleep 2s (wait for things to settle)
 * --> Read solar voltages -- switch each voltage through the 4052 to the single ADC (or, employ a TLC00834 or something)
 * --> Sleep 1s
 * --> Read DS18B20 sensor - in ground
 * --> Sleep 1s
 * --> Read DS18B20 sensor - surface
 * --> Sleep 1s
 * --> Turn on wifi
 * --> On connection successful, send report to remote
 * --> On error, flash error LED
 * --> Go back to deep sleep for (configured interval) 30s
 *     / or, sleep (configuragble 30s) then reset to simulate deep sleep wake
 *
 *
 * Calibration
 * -----------
 * If multiple 1-wire devices are chained on same GPIO we need to 'learn' which is which.
 * This can be done using dip-switches and 'cutting in' only one then teaching the device which is which.
 * It can then save the mapping into EEPROM.
 * User can indicate when prompted by a colour flashing LED.
 *
 *
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

#define MSG_BAR "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"

#define MSG_SIMULATION(X) console_printf("\n" MSG_BAR "\n[SIMLN]" X "\n" MSG_BAR "\n\n")

#define MSG_TRACE(X ...) console_printf("\n[TRACE]" X )

#define MSG_ERROR(X ...) console_printf("\n[ERROR]" X )

/* ------------------------------------------------------------------------- */
/* Configuration values, initialised from environment in init() */
struct sentrifarm_cfg_t {
  int settle_wait_s;    /* How long to wait from boot until first set of sensor readings */
  int thermal_wait_s ;  /* How long to wait from first set of sensor readings until each next DS18B20 reading */
  int telemetry_wait_s; /* How long to wait after reading sensors until sending telemetry */
};

/* ------------------------------------------------------------------------- */
/* Possible states */
enum {
  STATE_BOOT = 0, 
  STATE_SENSORS,
  STATE_THERMAL_SUBS,
  STATE_THERMAL_GRND,
  STATE_TELEMETRY,
  STATE_DONE,
  STATE_ERROR = -1 };

/* ------------------------------------------------------------------------- */
/* Configuration values, initialised from environment in init() */
struct sentrifarm_state_t {
  int state;            /* State in state machine at completion of timer callback */
};

/* ------------------------------------------------------------------------- */
/* Configuration */
static struct sentrifarm_cfg_t my_config = { 3, 1, 1 };

/* ------------------------------------------------------------------------- */
/* State machine */
static struct sentrifarm_state_t my_state = { STATE_BOOT };

/* ------------------------------------------------------------------------- */
/* State machine timer */
static os_timer_t sentri_state_timer;

static void sentri_state_timer_cb(void* unused);

/* ------------------------------------------------------------------------- */
/* Something went wrong, flash the error LED */
static void sentri_handle_state_error()
{
  MSG_ERROR("Invalid state: %d\n", my_state.state);
  // flash LED here
}

/* ------------------------------------------------------------------------- */
/* Read all voltages and digital I/O */
static void sentri_read_sensors()
{
  MSG_SIMULATION("Read solar voltage");
  MSG_SIMULATION("Read supercap voltage");
  MSG_SIMULATION("Read regulator input voltage");
  MSG_SIMULATION("Read regulator output voltage");
  MSG_SIMULATION("Read diagnostic switch");
}

/* ------------------------------------------------------------------------- */
/* Read DS18B20 */
static void sentri_read_thermal()
{
  switch (my_state.state) {
  case STATE_THERMAL_SUBS:
    MSG_SIMULATION("Read subsurface temperature");
    break;
  case STATE_THERMAL_GRND:
    MSG_SIMULATION("Read ground temperature");
    break;
  }
}

/* ------------------------------------------------------------------------- */
static void sentri_do_telemetry()
{
  MSG_SIMULATION("Telemetry");
}

/* ------------------------------------------------------------------------- */
/* State machine handler.
 * We could have gone overboard and used a look table machine type thing, maybe later...
 */
static void sentri_state_handler()
{
  /* We dont actually change the state until just before we set the next timer */
  /* Which means on input to this function we have the state at the point of read, so the functions can reference it as well */
  int next_state = STATE_ERROR;
  int next_delay = 0; /* 0 == no timer... */
  switch (my_state.state) {

  case STATE_BOOT:
    /* This is called directly from init... */
    MSG_TRACE("STATE_BOOT");
    next_state = STATE_SENSORS;
    next_delay = my_config.settle_wait_s;
    break;

  case STATE_SENSORS:
    MSG_TRACE("STATE_SENSORS");
    sentri_read_sensors();
    next_state = STATE_THERMAL_SUBS;
    next_delay = my_config.thermal_wait_s;
    break;

  case STATE_THERMAL_SUBS:
    MSG_TRACE("STATE_THERMAL_SUBS");
    sentri_read_thermal();
    next_state = STATE_THERMAL_GRND;
    next_delay = my_config.thermal_wait_s;
    break;

  case STATE_THERMAL_GRND:
    MSG_TRACE("STATE_THERMAL_GRND");
    sentri_read_thermal();
    next_state = STATE_TELEMETRY;
    next_delay = my_config.telemetry_wait_s;
    break;

  case STATE_TELEMETRY:
    MSG_TRACE("STATE_TELEMETRY");
    sentri_do_telemetry();
    next_state = STATE_DONE;
    break;

  default:
    sentri_handle_state_error();
    /* For the moment we become a stock frankenstein */
    /* TODO: Consider a watchdog reset */
    return;
  }
  my_state.state = next_state;
  os_timer_disarm(&sentri_state_timer);
  if (next_delay > 0) {
    MSG_TRACE("Timer: %d\n", next_delay);
    os_timer_setfn(&sentri_state_timer, (os_timer_func_t *)sentri_state_timer_cb, NULL);
    os_timer_arm(&sentri_state_timer, next_delay*1000, 0);
  } else {
    MSG_TRACE("Timer: none\n");
  }
}

/* ------------------------------------------------------------------------- */
/* State machine timer.
 * We could have gone overboard and used a look table machine type thing, maybe later...
 */
static void sentri_state_timer_cb(void* unused)
{
  sentri_state_handler();
}

/* ------------------------------------------------------------------------- */
/* Handler for poll timer */
static void deleted_function(void*unused)
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
//    console_printf("Sensor Error\n", buf);
    execute_tcp_push(DEMO_IPADDRESS, port, "{\"x\":\"error\"}");
  }
}


/* This function is called at the end of the init_done() function installed by user_init() */
void sentrifarm_init()
{
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

  const char* s_env = NULL;
  int v = 0;
	s_env = env_get("sentri-settle-t");    if (s_env && (v=atoi(s_env))>0) { my_config.settle_wait_s = v; }
	s_env = env_get("sentri-thermal-t");   if (s_env && (v=atoi(s_env))>0) { my_config.thermal_wait_s = v; }
	s_env = env_get("sentri-telemetry-t"); if (s_env && (v=atoi(s_env))>0) { my_config.telemetry_wait_s = v; }

  console_printf("SentriFarm Configuration: delays:%d,%d,%d\n", my_config.settle_wait_s, my_config.thermal_wait_s, my_config.telemetry_wait_s);

  MSG_SIMULATION("Read abort switch");
  MSG_SIMULATION("Read diagnostic switch");

  MSG_TRACE("Configure Done");

  my_state.state = STATE_BOOT;
  sentri_state_handler();
}

/* ------------------------------------------------------------------------- */
/* ESP-03 module notes

  * A capacitor across Vcc - GND would be useful
  * Unlike ESP-01, RST not initially connected to anything
  * Theory, normally GPIO16 'pin' drives XPD_DCDC for deep sleep, has pad for link or resistor (needed for FTDI module to do reset) (400ohm) to reset N/C
  * For some reason need to boot with CH_PD->pullup, GPIO15->GND
  * New module needs to be reflashed: CH_PD->pullup, GPIO15->GND, GPIO0->GND, GPIO2->pullup
*/
