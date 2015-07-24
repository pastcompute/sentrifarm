#ifndef HOSTNAME_H__
#define HOSTNAME_H__

/* This is required by lwip/opt.h which has been patched to search for this function */
const char* fr_request_hostname(void);

#endif
