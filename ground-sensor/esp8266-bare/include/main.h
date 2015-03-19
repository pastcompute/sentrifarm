
#ifndef _MAIN_H
#define _MAIN_H

/*
   !!
	DO NOT USE MALLOC
   !!
  
   USE MEM_ALLOC INSTEAD (MEM_ALLOC/MEM_REALLOC/MEM_FREE)
   which are defined in lwip/mem.h and defined as PvPort*...
   or os_*alloc()...
*/


/* This is required by lwip/opt.h which has been patched to search for this function */
const char* fr_request_hostname(void);

#endif // _MAIN_H
