#ifndef TCP_PUSH_H__
#define TCP_PUSH_H__

#include <lwip/ip_addr.h>

typedef void (*tcp_push_error_fcn_t)(void* handle, int error);
typedef void (*tcp_push_finish_fcn_t)(void* handle);

extern int execute_tcp_push(const char *ipaddress, int port, const char *text, tcp_push_finish_fcn_t finish_fcn, tcp_push_error_fcn_t error_fcn, void** handle);
extern int execute_tcp_push_u(ip_addr_t target, int port, const char *text, tcp_push_finish_fcn_t finish_fcn, tcp_push_error_fcn_t error_fcn, void** handle);

#endif
