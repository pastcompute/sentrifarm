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
 * Flow of Events
 * --------------
 *
 * Boot / wake from deep sleep --> sentrifarm_init()
 * --> If GPIOx (bypass switch) is connected to GND, then skip sentrifram processing and become a normal frankenstein
 * --> If GPIOx (debug mode switch) is connected to GND, then turn on debug LED
 * --> If GPIOx : DIAG1 (debug mode switch) is connected to GND, then dont reboot / deep sleep after completing logic flow
 *     This will be GPIO2 if testing limited system on ESP-01
 * --> Ensure wifi is off (should be boot disabled)
 * --> Enable sensor power
 * --> Sleep 2s (wait for things to settle)
 * --> Read solar voltages -- switch each voltage through the 4052 to the single ADC (or, employ a TLC00834 or something)
 * --> Read DS18B20 sensors
 * --> Disable sensor power
 * --> Sleep 2s (wait for things to settle)
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
#include "ds18b20.h"
#include "iwconnect.h"
#include "gpio.h"
#include "pin_map.h"

/* avoid annoying warnings */
extern void ets_intr_lock();
extern void ets_intr_unlock();
extern uint32_t readvdd33(void);

#define SENTRI_GPIO_DS18B20_STRING1 0
#define SENTRI_GPIO_DIAG1 2
#define SENTRI_GPIO_LED1 2
#define SENTRI_GPIO_BYPASS 2

/* 1 for ESP-01, 0 for module with several GPIO exposed */
#define TRAIT_SENSORS_ALWAYS_POWERED 1

/* 0 for ESP-01, 1 for module with several GPIO exposed */
#define TRAITS_ENABLE_LED 0

/* 0 for ESP-01, 1 for module with several GPIO exposed */
#define TRAITS_ENABLE_BYPASS 0

/* 1 for module with deep sleep GPIO16 connected to wake on reset */
#define TRAITS_ENABLE_DEEP_SLEEP 0

/* Number of DS18B20 expected to find on GPIO_DS18B20_STRING1 */
#define TRAIT_EXPECTED_THERMALS1 1

#define SENSOR_POLL_INTERVAL_SECONDS (60*30)
#define MAX_WIFIUP_RETRY_ATTEMPTS 20
#define MAX_TELEMETRY_RETRY_ATTEMPTS 20

#define DEMO_IPADDRESS "172.30.42.19"
#define DEMO_PORT 7777

#define MSG_BAR "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"

#define MSG_SIMULATION(X) console_printf("\n" MSG_BAR "\n[SIMLN]" X "\n" MSG_BAR "\n")

#define MSG_TRACE(X ...) console_printf("\n[TRACE]" X )

#define MSG_ERROR(X ...) console_printf("\n[ERROR]" X )

#define ARRAY_LEN(x) (sizeof(x)/sizeof(x[0]))

#define MAX_TELEMETRY 3

/* ------------------------------------------------------------------------- */
/* Configuration values, initialised from environment in init() */
struct sentrifarm_cfg_t {
  int wait_settle_s;       /* How long to wait after switching off wifi */
  int wait_sensors_s;      /* How long to wait after powering up sensors */
  int wait_wifiup_s;       /* How long to wait after reading sensors to start wifi, and between polling wifi status */
  int wait_transmit_s;     /* Wait for TCP transmission (until we implement event driven) */
  int wait_reboot_s;       /* Delay until reboot */
  ip_addr_t upstream_host; /* IPv4 address of base station service to send telemetry to */
  int upstream_port;       /* TCP/IP port for telemetry service */
  bool diag1;              /* True if diagnostic button #1 was held at boot */
  bool diag2;              /* True if diagnostic button #2 was held at boot */
  unsigned long deepsleep_us; /* microseconds to stay in deep sleep */
};

/* ------------------------------------------------------------------------- */
/* Configuration values, initialised from environment in init() */
static struct sentrifarm_cfg_t my_config = {
  .wait_settle_s = 5,           /* Allow 5 seconds for wifi to shutdown */
  .wait_sensors_s = 2,          /* Allow 2 seconds for sensor power to settle */
  .wait_wifiup_s = 1,           /* Allow 1 second between wifi polls */
  .wait_transmit_s = 1,         /* Allow 1 second between TCP polls */
  .wait_reboot_s = SENSOR_POLL_INTERVAL_SECONDS, /* For the moment, we set a timer then reboot, instead of deep sleep */
  .upstream_host = { 0 },
  .upstream_port = DEMO_PORT,
  .diag1 = false,
  .diag2 = false,
  .deepsleep_us = 1000 * 1000 * SENSOR_POLL_INTERVAL_SECONDS,
};

/* ------------------------------------------------------------------------- */
/* Possible states */
enum {
  STATE_BOOT = 0, 
  STATE_SETTLE, 
  STATE_SENSORS,
  STATE_WIFI_PENDING,
  STATE_TELEMETRY,
  STATE_TELEMETRY_PENDING,
  STATE_TELEMETRY_FAULT,
  STATE_DONE,
  STATE_REBOOT_REQUIRED,
  STATE_ERROR = -1
};

/* ------------------------------------------------------------------------- */
/* Various error codes, bit masked into 32-bits */
enum {
  ERR_OTHER                 = 1 << 0,
  ERR_THERMAL1_GPIO         = 1 << 1,
  ERR_THERMAL2_GPIO         = 1 << 2,
  ERR_WIFI_CONNECT          = 1 << 3,
  ERR_WIFI_AP_NOT_FOUND     = 1 << 4,
  ERR_THERMAL1_NONE_FOUND   = 1 << 5,
  ERR_THERMAL1_SOME_MISSING = 1 << 6,
};

/* ------------------------------------------------------------------------- */
/* System state and collected data */
struct sentrifarm_state_t {
  int state;                 /* State in state machine at completion of timer callback */
  uint32_t error_flags;      /* Accumulated error data */

  int thermals1_found;       /* Number of valid DS18B20 on GPIO #2 */
  struct ds18b20_t thermals1[TRAIT_EXPECTED_THERMALS1];  /* Temperatures on DS18B20 on GPIO #0 */
  uint32_t vdd;
  uint32_t ticks;

  int wifi_pending;          /* Latch to 1 once attempt to connect to wifi has started */
  int wifi_retried;          /* Number of elapsed wait_wifiup_s intervals since connection attempt started */

  int telemetry_retried;     /* Number of elapsed wait_transmit_s intervals since telemetry attempt started */
  void* telemetry_handle[MAX_TELEMETRY]; /* Handles of TCP transmissions. */
                             /* FIXME - we need to refactor TCP push to have one connect and multiple send, but this will do for the moment */
  bool telemetry_done[MAX_TELEMETRY]; /* true if specified message was sent or failed */
};

/* ------------------------------------------------------------------------- */
/* State machine and collected data */
static struct sentrifarm_state_t my_state = {
  .state = STATE_BOOT,
  .error_flags = 0,
  .thermals1_found = 0,
  .vdd = 0,
  .ticks = 0,
  .wifi_pending = 0,
  .wifi_retried = 0,
  .telemetry_retried = 0,
};

/* ------------------------------------------------------------------------- */
/* State machine timer */
static os_timer_t sentri_state_timer;

/* Forward decl. */
static void sentri_state_timer_cb(void* unused);

/* ------------------------------------------------------------------------- */
/* Something went wrong, flash the error LED */
static void sentri_handle_state_error()
{
  MSG_ERROR("Invalid state: %d", my_state.state);
  // flash LED here
}

/* ------------------------------------------------------------------------- */
/* Read DS18B20 */
static void sentri_read_thermal()
{
  /* Now, this first version is built on a ESP-01 which only has two GPIO.
     The DS18B20 allows multiplexing of multiple sensors onto one line,
     however, to be able to tell them apart either would require having to have a jig for
     reading the unique addresses and coding into an EEPROM accessible to this unit,
     or, some kind of calibration - perhaps a switch set which can be used to
     one at a time switch all but one sensor out of the circuit and have the
     ESP8266 determine for itself which is it
     For the moment both options are annoying, so we fake it.
   */
  MSG_SIMULATION("Read thermal sensors");
  my_state.thermals1_found = 0;
  if (!exec_ds18b20(SENTRI_GPIO_DS18B20_STRING1, my_state.thermals1, ARRAY_LEN(my_state.thermals1), &my_state.thermals1_found)) {
    my_state.error_flags |= ERR_THERMAL1_GPIO;
    MSG_ERROR("Invalid DS18B20 GPIO!");
    return;
  }
  if (my_state.thermals1_found == 0) {
    MSG_ERROR("No thermals1 found");
    my_state.error_flags |= ERR_THERMAL1_NONE_FOUND;
  } else if (my_state.thermals1_found < TRAIT_EXPECTED_THERMALS1) {
    MSG_ERROR("Not all thermals1 found");
    my_state.error_flags |= ERR_THERMAL1_SOME_MISSING;
  }
}

/* ------------------------------------------------------------------------- */
/* Read all voltages and digital I/O */
static void sentri_read_sensors()
{
  MSG_TRACE("Read VCC");
	os_intr_lock();
	my_state.vdd = readvdd33();
	os_intr_unlock();
  my_state.ticks = system_get_rtc_time();

  MSG_TRACE("Read solar voltage (TODO)");
  MSG_TRACE("Read supercap voltage (TODO)");
  MSG_TRACE("Read regulator input voltage (TODO)");
  sentri_read_thermal();
}

/* ------------------------------------------------------------------------- */
/* Read all voltages and digital I/O */
static void sentri_connect_wifi()
{
  /* This next code is copied from main.c */
  const char *ssid = env_get("sta-auto-ssid");
  const char *pass = env_get("sta-auto-password");
  if (ssid) {
    MSG_TRACE("STA mode: attempting connection to %s\n", ssid);
    exec_iwconnect(ssid, pass);
  }
}

/* ------------------------------------------------------------------------- */
static void sentri_tcp_finish_handler(void* handle)
{
  for (int i=0; i < ARRAY_LEN(my_state.telemetry_handle); i++) {
    if (handle == my_state.telemetry_handle[i]) {
      my_state.telemetry_done[i] = true;
      MSG_ERROR("TCP Finish %d", i);
      break;
    }
  }
}

/* ------------------------------------------------------------------------- */
static void sentri_tcp_fault_handler(void* handle, int error)
{
  for (int i=0; i < ARRAY_LEN(my_state.telemetry_handle); i++) {
    if (handle == my_state.telemetry_handle[i]) {
      my_state.telemetry_done[i] = true;
      MSG_ERROR("TCP Fault on msg %d :-(", i);
      break;
    }
  }
}

/* ------------------------------------------------------------------------- */
static void sentri_do_telemetry()
{
  /* For the moment just send a bunch of JSON to a dumb telnet-style logging service */
  /* TODO: do this properly - open one TCP connection then send through data until done */
  /* TODO: amongst other things, each message allocates a non-trivial struct on the heap... we probably want to avoid that */
  MSG_SIMULATION("Telemetry");
  for (int i=0; i < MAX_TELEMETRY; i++) { my_state.telemetry_handle[i] = NULL; my_state.telemetry_done[i] = true; }
  my_state.telemetry_retried = 0;
  int nextHandle = 0;
  if (true) {
    char buf[48];
    snprintf(buf, sizeof(buf), "{\"vdd\":\"%u\",\"ticks\":\"%u\",\"diag\":\"%u\"}", my_state.vdd, my_state.ticks, my_config.diag2);
    MSG_TRACE("%s", buf);
    my_state.telemetry_done[nextHandle] = false;
    execute_tcp_push_u(my_config.upstream_host, my_config.upstream_port, buf, &sentri_tcp_finish_handler, &sentri_tcp_fault_handler, &my_state.telemetry_handle[nextHandle++]);
  }
  for (int thermal=0; thermal < my_state.thermals1_found; thermal++) {
    const struct ds18b20_t* r = &my_state.thermals1[thermal];
    char buf[6+2*8+7+1+7+2+2];
    snprintf(buf, sizeof(buf), "{\"x\":\"%02x%02x%02x%02x%02x%02x%02x%02x\",\"t\":\"%c%d.%02d\"}",
        r->addr[0], r->addr[1], r->addr[2], r->addr[3], r->addr[4], r->addr[5], r->addr[6], r->addr[7], r->tSign, r->tVal, r->tFract);
    MSG_TRACE("%s", buf);
    my_state.telemetry_done[nextHandle] = false;
    execute_tcp_push_u(my_config.upstream_host, my_config.upstream_port, buf, &sentri_tcp_finish_handler, &sentri_tcp_fault_handler, &my_state.telemetry_handle[nextHandle++]);
  }
  if (true) {
    char buf[22];
    snprintf(buf, sizeof(buf), "{\"error\":\"%.08x\"}\n", my_state.error_flags); /* Last one, so add a CR */
    MSG_TRACE("%s", buf);
    my_state.telemetry_done[nextHandle] = false;
    execute_tcp_push_u(my_config.upstream_host, my_config.upstream_port, buf, &sentri_tcp_finish_handler, &sentri_tcp_fault_handler, &my_state.telemetry_handle[nextHandle++]);
  }
  /* On return, we poll the telemetry handles until all messages are sent */
}

/* ------------------------------------------------------------------------- */
static bool sentri_check_telemetry()
{
  bool finished = true;
  for (int i=0; i < ARRAY_LEN(my_state.telemetry_done); i++) {
    if (!my_state.telemetry_done[i]) finished = false;
  }
  return finished;
}

/* ------------------------------------------------------------------------- */
static void sentri_check_wifi()
{
	int state = wifi_station_get_connect_status();
  bool finished = 0;
  switch (state) {

  case STATION_CONNECT_FAIL:
    MSG_ERROR("Failed to connect to wifi - connect fail error :-(");
    my_state.error_flags |= ERR_WIFI_CONNECT;
    finished = true;
    break;

  case STATION_NO_AP_FOUND:
    MSG_ERROR("Failed to connect to wifi - AP not found :-(");
    my_state.error_flags |= ERR_WIFI_AP_NOT_FOUND;
    finished = true;
    break;

  case STATION_GOT_IP:
    MSG_TRACE("Wifi connected");
    finished = true;
    break;
  };
  if (finished) {
    my_state.wifi_pending = 0;
  }
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
    MSG_SIMULATION("Shutdown Wifi");
  	wifi_station_disconnect();
    next_state = STATE_SETTLE;
    next_delay = my_config.wait_settle_s;
    break;

  case STATE_SETTLE:
#if TRAITS_ENABLE_LED
  	GPIO_OUTPUT_SET(SENTRI_GPIO_LED1, 0);
#endif
    MSG_TRACE("STATE_SETTLE");
#if TRAIT_SENSORS_ALWAYS_POWERED
    MSG_SIMULATION("Skipping sensor powerup (ESP-01)");
    next_delay = 1;
#else
    MSG_SIMULATION("Powering up sensors");
    next_delay = my_config.wait_sensors_s;
#endif
    next_state = STATE_SENSORS;
    break;

  case STATE_SENSORS:
    MSG_TRACE("STATE_SENSORS");
    sentri_read_sensors();
    sentri_connect_wifi();
    my_state.wifi_pending = 1;
    my_state.wifi_retried = 0;
    next_state = STATE_WIFI_PENDING;
    next_delay = my_config.wait_wifiup_s;
    break;

  case STATE_WIFI_PENDING:
    sentri_check_wifi();
    next_state = STATE_WIFI_PENDING;
    next_delay = my_config.wait_wifiup_s;
    if (my_state.wifi_pending) {
      if (my_state.wifi_retried ++ > MAX_WIFIUP_RETRY_ATTEMPTS) { /* timeout ... */
        MSG_ERROR("Failed to connect to wifi in time :-(");
        next_state = STATE_REBOOT_REQUIRED;
        next_delay = my_config.wait_reboot_s;
        break;
      }
    } else {
      next_state = STATE_TELEMETRY;
      next_delay = my_config.wait_wifiup_s;
    }
    break;

  case STATE_TELEMETRY:
    MSG_TRACE("STATE_TELEMETRY");
    sentri_do_telemetry();
    next_state = STATE_TELEMETRY_PENDING;
    next_delay = my_config.wait_transmit_s;
    break;

  case STATE_TELEMETRY_PENDING:
    MSG_TRACE("STATE_TELEMETRY_PENDING");
    next_state = STATE_TELEMETRY_PENDING;
    next_delay = my_config.wait_transmit_s;
    if (!sentri_check_telemetry()) {
      if (my_state.telemetry_retried ++ > MAX_TELEMETRY_RETRY_ATTEMPTS) { /* timeout ... */
        MSG_ERROR("Failed to transmit telemetry in time :-(");
        next_state = STATE_TELEMETRY_FAULT;
        next_delay = my_config.wait_transmit_s;
      }
      break;
    }
    MSG_TRACE("TELEMETRY DONE");
    /* Disconnect wifi now in case shutting it down on the reboot is responsible for the random boot segfault */
  	wifi_station_disconnect();
    next_state = STATE_DONE;
    next_delay = 1;
    break;

  case STATE_TELEMETRY_FAULT:
    MSG_TRACE("STATE_TELEMETRY_FAULT");
    /* Here we could perhaps do a 60 second try again before rebooting - dont want to waste all the power because of a network transient */
    next_state = STATE_REBOOT_REQUIRED;
    next_delay = my_config.wait_reboot_s;
    break;   

  case STATE_DONE:
    MSG_TRACE("DONE");
#if TRAITS_ENABLE_DEEP_SLEEP
    if (my_config.diag2) {
      MSG_TRACE("DEEP SLEEP aborted by diagnostic button #2");
      os_timer_disarm(&sentri_state_timer);
      return;
    }
    MSG_TRACE("Nap Time");
    os_timer_disarm(&sentri_state_timer);
    system_deep_sleep(my_config.deepsleep_us);
    return;
#else
    next_state = STATE_REBOOT_REQUIRED;
    next_delay = my_config.wait_reboot_s;
#endif
    break;

  case STATE_REBOOT_REQUIRED:
    if (my_config.diag2) {
      MSG_TRACE("REBOOT aborted by diagnostic button #2");
    } else {
      MSG_SIMULATION("REBOOTING");
      system_restart();
    }
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
    MSG_TRACE("Timer: %d", next_delay);
    os_timer_setfn(&sentri_state_timer, (os_timer_func_t *)sentri_state_timer_cb, NULL);
    os_timer_arm(&sentri_state_timer, next_delay*1000, 0);
  } else {
    MSG_TRACE("Timer: none");
    /* For the moment we become a stock frankenstein. We get here if diag button was held */
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
/* This function is called at the end of the init_done() function installed by user_init() */
void sentrifarm_init()
{
#if TRAITS_ENABLE_BYPASS
  GPIO_DIS_OUTPUT(SENTRI_GPIO_BYPASS);
	PIN_FUNC_SELECT(pin_mux[SENTRI_GPIO_BYPASS], pin_func[SENTRI_GPIO_BYPASS]);
	PIN_PULLUP_EN(pin_mux[SENTRI_GPIO_BYPASS]);
  if (!GPIO_INPUT_GET(SENTRI_GPIO_BYPASS)) {
    console_printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    console_printf("~ SENTRIFARM DISABLED                     ~\n");
    console_printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    return;
  }
#endif

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

  ipaddr_aton(DEMO_IPADDRESS, &my_config.upstream_host);
  my_state.state = STATE_BOOT;
  my_state.error_flags = 0;
  my_state.thermals1_found = 0;
  my_state.wifi_pending = 0;
  my_state.wifi_retried = 0;
  my_state.telemetry_retried = 0;
  
  const char* s_env = NULL;
  int v = 0;
	s_env = env_get("sentri-settle-t");      if (s_env && (v=atoi(s_env))>0) { my_config.wait_settle_s = v; }
	s_env = env_get("sentri-sensors-t");     if (s_env && (v=atoi(s_env))>0) { my_config.wait_sensors_s = v; }
	s_env = env_get("sentri-wifiup-t");      if (s_env && (v=atoi(s_env))>0) { my_config.wait_wifiup_s = v; }
	s_env = env_get("sentri-upstream-port"); if (s_env && (v=atoi(s_env))>0) { my_config.upstream_port = v; }
  s_env = env_get("sentri-upstream-host"); if (s_env && strlen(s_env) > 0) { ipaddr_aton(s_env, &my_config.upstream_host); }

  char buf[128]; ipaddr_ntoa_r(&my_config.upstream_host, buf, sizeof(buf));
  const char *ssid = env_get("sta-auto-ssid");

  MSG_SIMULATION("Flash LED");
#if TRAITS_ENABLE_LED
 	GPIO_OUTPUT_SET(SENTRI_GPIO_LED1, 1);
#endif
  MSG_SIMULATION("Read abort switch");       /* <-- set my_config.diag1 here */
  MSG_SIMULATION("Read diagnostic switch");  /* <-- set my_config.diag2 here */

  /* Pull-up internally; need to short to enable DIAG1 mode */
  GPIO_DIS_OUTPUT(SENTRI_GPIO_DIAG1);
	PIN_FUNC_SELECT(pin_mux[SENTRI_GPIO_DIAG1], pin_func[SENTRI_GPIO_DIAG1]);
	PIN_PULLUP_EN(pin_mux[SENTRI_GPIO_DIAG1]);
  my_config.diag2 = !GPIO_INPUT_GET(SENTRI_GPIO_DIAG1);

  console_printf("SentriFarm Configuration: delays=%d,%d,%d; upstream=%s:%d ssid=%s, diag2=%d\n",
        my_config.wait_settle_s, my_config.wait_sensors_s, my_config.wait_wifiup_s,
        buf, my_config.upstream_port, ssid, my_config.diag2);

  MSG_TRACE("Configure Done");

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

/* Prerequisite configuration:

sta-auto=1
sta-auto-ssid=...
sta-auto-password=...

*/

