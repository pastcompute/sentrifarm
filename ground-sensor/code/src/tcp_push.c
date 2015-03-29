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

#include <stdlib.h>
#include <generic/macros.h>

#define CONFIG_TCP_PUSH_DEBUG
#ifdef CONFIG_TCP_PUSH_DEBUG
#define dbg(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

struct pokerface {
  struct espconn esp_conn;
  esp_tcp esptcp;
  char databuf[128];
  int datalen;
  /*volatile*/ os_timer_t conn_checker;
};

static void conn_checker_handler(void *arg)
{
  struct pokerface *p = arg;
  /* Lazy gc */
  os_free(p);
  dbg("free\n");
}

static void  connected(void *arg)
{
  struct pokerface *p = arg;
  espconn_sent(&p->esp_conn, (uint8*)p->databuf, p->datalen);
}

static void  disconnected(void *arg)
{
  dbg("OK\n");
  console_lock(0);
  struct pokerface *p = arg;
  os_timer_arm(&p->conn_checker, 50, 0);
  
}

static void reconnect(void *arg, sint8 err)
{
  console_printf("Error %d\n", err);
  struct pokerface *p = arg;
  espconn_disconnect(&p->esp_conn);
  console_lock(0);
}

static void datasent(void *arg)
{
  struct pokerface *p = arg;
  os_timer_disarm(&p->conn_checker);
  os_timer_setfn(&p->conn_checker, (os_timer_func_t *)conn_checker_handler, p);
  espconn_disconnect(&p->esp_conn);
}

/* At most, 128 bytes! */
int execute_tcp_push(const char *ipaddress, int port, const char *text)
{
  struct pokerface *p = os_zalloc(sizeof(struct pokerface));
  if (!p || !text || port==0 || !ipaddress) {
    console_printf("Can't malloc enough to send\n");
    return -1;
  }
  if (strlen(text)+1 > sizeof(p->databuf)) {
    console_printf("Too much data\n");
    return -1;
  }
  
  p->esp_conn.type = ESPCONN_TCP;
  p->esp_conn.state = ESPCONN_NONE;
  p->esp_conn.proto.tcp = &p->esptcp;
  p->esp_conn.proto.tcp->local_port = espconn_port();
  p->esp_conn.proto.tcp->remote_port = port;
  uint32_t target = ipaddr_addr(ipaddress);
  
  strcat(p->databuf, text);
  p->datalen = strlen(p->databuf);
  
  os_memcpy(p->esp_conn.proto.tcp->remote_ip, &target, 4);
  espconn_regist_connectcb(&p->esp_conn, connected);  
  espconn_regist_reconcb(&p->esp_conn, reconnect);
  espconn_regist_disconcb(&p->esp_conn, disconnected);
  espconn_regist_sentcb(&p->esp_conn, datasent);
  espconn_connect(&p->esp_conn);
  console_lock(1);

  return 0;
}

/*
static void do_send_interrupt(void)
{

}
*/
