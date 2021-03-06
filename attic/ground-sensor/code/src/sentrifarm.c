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
 * ESP-01 Deep Sleep
 * -----------------
 *
 * This requires soldering a wire between pin 8, XPD_DCDC and RESET (http://tim.jagenberg.info/2015/01/18/low-power-esp8266/)
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

#include "env.h"
#include "ds18b20.h"
#include "tcp_push.h"
#include "iwconnect.h"
#include "gpio.h"
#include "pin_map.h"
#include "helpers.h"

#include <stdio.h>
#include <stdlib.h>

/* avoid annoying warnings - for some reason therse are not in the system include files */
extern void ets_intr_lock();
extern void ets_intr_unlock();
extern uint32_t readvdd33(void);

/* GPIO mappings. The following suit all modules but limited functionality to a DS18B20 and a switch and LED */
#define SENTRI_GPIO_DS18B20_STRING1 0
#define SENTRI_GPIO_DIAG2 2
#define SENTRI_GPIO_LED1 2
#define SENTRI_GPIO_BYPASS 2

/* 1 for ESP-01, 0 for module with several GPIO exposed */
#define TRAIT_SENSORS_ALWAYS_POWERED 1

/* 0 for ESP-01, 1 for module with several GPIO exposed */
#define TRAITS_ENABLE_LED 1

/* 0 for ESP-01, 1 for module with several GPIO exposed */
#define TRAITS_ENABLE_BYPASS 0

/* 1 for module with deep sleep GPIO16 connected to wake on reset */
#define TRAITS_ENABLE_DEEP_SLEEP 1

/* Number of DS18B20 expected to find on GPIO_DS18B20_STRING1 */
#define TRAIT_EXPECTED_THERMALS1 1

/* How long to deep sleep / timer for between data collection and broadcast */
#define SENSOR_POLL_INTERVAL_SECONDS (60*30)

/* Timeout counters for various error conditions (eff. seconds) */
#define MAX_WIFIUP_RETRY_ATTEMPTS 22
#define MAX_TELEMETRY_RETRY_ATTEMPTS 20

/* Default IP address and port to use if not configured in the environment */
#define DEMO_IPADDRESS "192.168.1.1"
#define DEMO_PORT 7777

/* For the moment just debug to the console, TODO: integrate into the configurable debug level system */
#define MSG_TRACE(X ...) console_printf("\n[DEBUG]" X )

/* Always report errros to the console */
#define MSG_ERROR(X ...) console_printf("\n[ERROR]" X )

/* Helper to safely get array length */
#define ARRAY_LEN(x) (sizeof(x)/sizeof(x[0]))

/* Maximum number of telemetry messages. FIXME the way telemetry currently works is a HACK! */
#define MAX_TELEMETRY 3

/* ------------------------------------------------------------------------- */
/* Configuration values, initialised from environment in init() */
struct sentrifarm_cfg_t {
  int wait_settle_s;           /* How long to wait after switching off wifi */
  int wait_sensors_s;          /* How long to wait after powering up sensors */
  int wait_wifiup_s;           /* How long to wait after reading sensors to start wifi, and between polling wifi status */
  int wait_transmit_s;         /* Wait for TCP transmission (until we implement event driven) */
  int wait_reboot_s;           /* Delay until reboot */
  ip_addr_t upstream_host;     /* IPv4 address of base station service to send telemetry to */
  int upstream_port;           /* TCP/IP port for telemetry service */
  bool diag1;                  /* True if diagnostic button #1 was held at boot */
  bool diag2;                  /* True if diagnostic button #2 was held at boot */
  unsigned long deepsleep_us;  /* microseconds to stay in deep sleep */
  const char *ssid;
  const char *password;
};

/* ------------------------------------------------------------------------- */
/* Configuration values, initialised from environment in init() */
static struct sentrifarm_cfg_t my_config = {
  .wait_settle_s = 3,           /* Allow time for wifi to shutdown */
  .wait_sensors_s = 2,          /* Allow time for sensor power to settle */
  .wait_wifiup_s = 1,           /* Allow 1 second between wifi polls */
  .wait_transmit_s = 1,         /* Allow 1 second between TCP polls */
  .wait_reboot_s = SENSOR_POLL_INTERVAL_SECONDS, /* For the moment, we set a timer then reboot, instead of deep sleep */
  .upstream_host = { 0 },       /* This needs to be set using the relevant conversion function */
  .upstream_port = DEMO_PORT,
  .diag1 = false,               /* True if diagnostic 1 was on at reboot */
  .diag2 = false,               /* True if diagnostic 1 was on at reboot */
  .deepsleep_us = 1000 * 1000 * SENSOR_POLL_INTERVAL_SECONDS, /* Deep sleep time in microseconds */
  .ssid = "sentri",
  .password = ""
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
  int state;                    /* State in state machine at completion of timer callback */
  uint32_t error_flags;         /* Accumulated error data */

  int thermals1_found;          /* Number of valid DS18B20 on GPIO #2 */
  struct ds18b20_t thermals1[TRAIT_EXPECTED_THERMALS1];  /* Temperatures on DS18B20 on GPIO #0 */
  uint32_t vdd;                 /* Measured system voltage */
  uint32_t ticks;               /* Current RTC count. In theory this persists through deep sleep, but I dont see it... */

  int wifi_pending;             /* Latch to 1 once attempt to connect to wifi has started */
  int wifi_retried;             /* Number of elapsed wait_wifiup_s intervals since connection attempt started */

  int telemetry_retried;        /* Number of elapsed wait_transmit_s intervals since telemetry attempt started */
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

/* Forward declaration for time callback */
static void sentri_state_timer_cb(void* unused);

/* ------------------------------------------------------------------------- */
/* Something went wrong, flash the error LED */
static void sentri_handle_state_error()
{
  MSG_ERROR("Invalid state: %d", my_state.state);
  /* FIXME: flash a LED here */
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
  os_intr_lock(); // Crapping out when wifi not disconnected?
  my_state.vdd = readvdd33();
  os_intr_unlock();
  MSG_TRACE("Read RTC");
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
  MSG_TRACE("STA mode: attempting connection to %s\n", my_config.ssid);
  wifi_set_phy_mode(PHY_MODE_11B);
  wifi_set_opmode(STATION_MODE);
  wifi_station_dhcpc_start();
  os_delay_us(250*1000);
  exec_iwconnect(my_config.ssid, my_config.password);
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
      MSG_ERROR("TCP Fault on msg %d :-(\n", i);
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
  for (int i=0; i < MAX_TELEMETRY; i++) { my_state.telemetry_handle[i] = NULL; my_state.telemetry_done[i] = true; }
  my_state.telemetry_retried = 0;
  int nextHandle = 0;
  if (true) {
    char buf[48 + (6+2*8+7+1+7+2+2) * my_state.thermals1_found + 22];
    snprintf(buf, sizeof(buf), "{\"vdd\":\"%u\",\"ticks\":\"%u\",\"diag\":\"%u\"}", my_state.vdd, my_state.ticks, my_config.diag2);
    for (int thermal=0; thermal < my_state.thermals1_found; thermal++) {
      const struct ds18b20_t* r = &my_state.thermals1[thermal];
      snprintf(buf+strlen(buf), sizeof(buf), "{\"x\":\"%02x%02x%02x%02x%02x%02x%02x%02x\",\"t\":\"%c%d.%02d\"}",
        r->addr[0], r->addr[1], r->addr[2], r->addr[3], r->addr[4], r->addr[5], r->addr[6], r->addr[7], r->tSign, r->tVal, r->tFract);
    }
    snprintf(buf+strlen(buf), sizeof(buf), "{\"error\":\"%.08x\"}\n", my_state.error_flags); /* Last one, so add a CR */

    MSG_TRACE("%s", buf);
    my_state.telemetry_done[nextHandle] = false;
    execute_tcp_push_u(my_config.upstream_host, my_config.upstream_port, buf, &sentri_tcp_finish_handler, &sentri_tcp_fault_handler, &my_state.telemetry_handle[nextHandle++]);
  }
  /* On return, we poll the telemetry handles until all messages are sent. We had more than one but that didnt work so well if sending took a bit of time */
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
    MSG_ERROR("Failed to connect to wifi - connect fail error :-(\n");
    my_state.error_flags |= ERR_WIFI_CONNECT;
    finished = true;
    break;

  case STATION_NO_AP_FOUND:
    MSG_ERROR("Failed to connect to wifi - AP not found :-(\n");
    my_state.error_flags |= ERR_WIFI_AP_NOT_FOUND;
    finished = true;
    break;

  case STATION_GOT_IP:
    MSG_TRACE("Wifi connected.");
    finished = true;
    break;
  };
  if (finished) {
    my_state.wifi_pending = 0;
  }
}

/* ------------------------------------------------------------------------- */
static void dump_state()
{
  uint8 f = system_get_cpu_freq();
  console_printf("CPU frequency=%dMHz\n", (int)f);

  unsigned char macaddr[6] = { 0,0,0,0,0,0 };
  unsigned char macaddr2[6] = { 0,0,0,0,0,0 };
  wifi_get_macaddr(SOFTAP_IF, macaddr);
  wifi_get_macaddr(STATION_IF, macaddr);
  console_printf("sdk=%s chipid=0x%x ap.mac=" MACSTR " sta.mac=" MACSTR " heap=%d tick=%u rtc.tick=%u\n",
    system_get_sdk_version(), system_get_chip_id(), MAC2STR(macaddr), MAC2STR(macaddr2),
    system_get_free_heap_size(), system_get_time(), system_get_rtc_time());

  console_printf("WIFI opmode=%s sleep=%s auto=%s status=%s phy=%s dhcp=%s\n",
      id_to_wireless_mode(wifi_get_opmode()), id_to_wifi_sleep_type(wifi_get_sleep_type()),
      wifi_station_get_auto_connect()?"yes":"no", id_to_sta_state(wifi_station_get_connect_status()),
      id_to_phy_mode(wifi_get_phy_mode()), wifi_station_dhcpc_status()==DHCP_STARTED?"started":"stopped"); 
  console_printf("vdd=%u ticks=%u diag=%u\n", my_state.vdd, my_state.ticks, my_config.diag2);
  for (int thermal=0; thermal < my_state.thermals1_found; thermal++) {
    const struct ds18b20_t* r = &my_state.thermals1[thermal];
    console_printf("%02x%02x%02x%02x%02x%02x%02x%02x : %c%d.%02d\n",
      r->addr[0], r->addr[1], r->addr[2], r->addr[3], r->addr[4],
      r->addr[5], r->addr[6], r->addr[7], r->tSign, r->tVal, r->tFract);
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

  MSG_TRACE("Handling state: %d", my_state.state);

  switch (my_state.state) {

  case STATE_BOOT:
    /* This is called directly from init... */
    next_state = STATE_SETTLE;
    next_delay = my_config.wait_settle_s;
    break;

  case STATE_SETTLE:
#if TRAITS_ENABLE_LED
    GPIO_OUTPUT_SET(SENTRI_GPIO_LED1, 0);
#endif
#if TRAIT_SENSORS_ALWAYS_POWERED
    MSG_TRACE("Sensors always powered on");
    next_delay = 1;
#else
    MSG_TRACE("TODO: Power up sensors");
    next_delay = my_config.wait_sensors_s;
#endif
    next_state = STATE_SENSORS;
    break;

  case STATE_SENSORS:
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
#if TRAITS_ENABLE_LED
      GPIO_OUTPUT_SET(SENTRI_GPIO_LED1, 1);
      os_delay_us(75*1000);
      GPIO_OUTPUT_SET(SENTRI_GPIO_LED1, 0);
#endif
      if (my_state.wifi_retried ++ > MAX_WIFIUP_RETRY_ATTEMPTS) { /* timeout ... */
        dump_state();
        MSG_ERROR("Failed to connect to wifi in time :-(");
        MSG_ERROR("Fallback to frankenstein console");
      	console_lock(0);
        next_state = -1;
        next_delay = 0;
        break;
      }
    } else {
      if (my_state.error_flags & ERR_WIFI_AP_NOT_FOUND) {
        dump_state();
        MSG_ERROR("Failed to connect to wifi AP :-(");
        MSG_ERROR("You have 30 seconds at the console before reset...\n");
      	console_lock(0);
        next_state = STATE_REBOOT_REQUIRED;
        next_delay = 30;
      } else {
        next_state = STATE_TELEMETRY;
        next_delay = my_config.wait_wifiup_s;
      }
    }
    break;

  case STATE_TELEMETRY:
    sentri_do_telemetry();
    next_state = STATE_TELEMETRY_PENDING;
    next_delay = my_config.wait_transmit_s;
    break;

  case STATE_TELEMETRY_PENDING:
    next_state = STATE_TELEMETRY_PENDING;
    next_delay = my_config.wait_transmit_s;
    if (!sentri_check_telemetry()) {
      if (my_state.telemetry_retried ++ > MAX_TELEMETRY_RETRY_ATTEMPTS) { /* timeout ... */
        MSG_ERROR("Failed to transmit telemetry in time :-(\n");
        next_state = STATE_TELEMETRY_FAULT;
        next_delay = my_config.wait_transmit_s;
      }
      break;
    }
    /* Note: we are not handling TCP error 11 properly, it bypasses TELEMETRY_FAULT in that case */

    /* Disconnect wifi now in case shutting it down on the reboot is responsible for the random boot segfault */
    /* http://www.esp8266.com/viewtopic.php?p=9559#p9559 */
    wifi_station_disconnect();
    next_state = STATE_DONE;
    next_delay = 3; // allow everything to disconnect...
    break;

  case STATE_TELEMETRY_FAULT:
    MSG_TRACE("STATE_TELEMETRY_FAULT\n");
    /* Here we could perhaps do a 60 second try again before rebooting - dont want to waste all the power because of a network transient */
    next_state = STATE_REBOOT_REQUIRED;
    next_delay = my_config.wait_reboot_s;
    break;

  case STATE_DONE:
#if TRAITS_ENABLE_DEEP_SLEEP
    if (my_config.diag2) {
      MSG_TRACE("DEEP SLEEP aborted by diagnostic button #2\n");
      os_timer_disarm(&sentri_state_timer);
      return;
    }
    MSG_TRACE("Nap Time");
    os_timer_disarm(&sentri_state_timer);
    /* Note: we can use system_rtc_mem_write() to retain data through deep sleep */
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
      MSG_TRACE("REBOOTING");
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
    // MSG_TRACE("Timer: %d", next_delay);
    os_timer_setfn(&sentri_state_timer, (os_timer_func_t *)sentri_state_timer_cb, NULL);
    os_timer_arm(&sentri_state_timer, next_delay*1000, 0);
  } else {
    MSG_TRACE("Leaving state machine\n");
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
  /* Ideally, the environment should be configured such that we dont attempt immediate connect at boot... */
  // FOR THE MOMENT THIS IS NOT WORKING WITH 'sentri' it was working on my other router WTF?
  // if (boot mode not from deep sleep)
  wifi_station_set_auto_connect(0);  // we do it manually...
  wifi_set_opmode(STATION_MODE);
  wifi_station_disconnect(); // if not already (but should be)

  /* Ensure RF is off until we turn it on after next wake up */
  system_deep_sleep_set_option(4);  // 4 - disable RF after wake-up. but how do we turn it back on 2 - no RF cal 1 - RF Cal (large current) 0 depends on byte 108 (?)

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
  s_env = env_get("sentri-sleep-t") ;      if (s_env && (v=atoi(s_env))>0) { my_config.deepsleep_us = v * 1000 * 1000; }
  s_env = env_get("sentri-upstream-port"); if (s_env && (v=atoi(s_env))>0) { my_config.upstream_port = v; }
  s_env = env_get("sentri-upstream-host"); if (s_env && strlen(s_env) > 0) { ipaddr_aton(s_env, &my_config.upstream_host); }

  /* This next code is copied from main.c */
  s_env = env_get("sentri-ssid");          if (s_env && strlen(s_env) > 0) { my_config.ssid = s_env; }
  s_env = env_get("sentri-password");      if (s_env && strlen(s_env) > 0) { my_config.password = s_env; }

  char buf[128]; ipaddr_ntoa_r(&my_config.upstream_host, buf, sizeof(buf));

  /* Pull-up internally; need to short to enable DIAG1 mode. Currently GPIO2 (same as bypass!) */
  console_printf("Short GPIO2 now to bypass deep sleep on completion...\n");
  os_delay_us(500*1000);
  GPIO_DIS_OUTPUT(SENTRI_GPIO_DIAG2);
  PIN_FUNC_SELECT(pin_mux[SENTRI_GPIO_DIAG2], pin_func[SENTRI_GPIO_DIAG2]);
  PIN_PULLUP_EN(pin_mux[SENTRI_GPIO_DIAG2]);
  my_config.diag2 = !GPIO_INPUT_GET(SENTRI_GPIO_DIAG2);
  if (my_config.diag2) {
    console_printf("Bypass deep sleep on completion!\n");
  }

#if TRAITS_ENABLE_LED
  /* Switch GPIO2 to output now, for controlling LED. Turn it on, then off again in a bit (~ 3 seconds) */
  PIN_PULLUP_DIS(pin_mux[SENTRI_GPIO_LED1]);
  PIN_PULLDWN_EN(pin_mux[SENTRI_GPIO_LED1]);
  GPIO_OUTPUT_SET(SENTRI_GPIO_LED1, 1);
#endif

  console_printf("SentriFarm Configuration: delays=%d,%d,%d; upstream=%s:%d ssid=%s, diag2=%d\n",
        my_config.wait_settle_s, my_config.wait_sensors_s, my_config.wait_wifiup_s,
        buf, my_config.upstream_port, my_config.ssid, my_config.diag2);

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
/* How to replace tx/rx with GPIO:
PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3);
PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1);
gpio_output_set(BIT3, 0, BIT3, 0);
gpio_output_set(0, BIT3, 0, BIT3);

*/
// Gyro/reed switches for buffeting instead of wind
