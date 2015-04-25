#objects-y+=sentrifarm.o
#objects-y+=ds18b20.o tcp_push.o

objects-y+=test_reassoc_after_sleep.o
objects-y+=cal_dex.o

ifneq ($(SENTRI_STANDALONE),y)
subdirs-y+=../frankenstein/src/
else
#objects-y+=entry.o
objects-y+=env.o   # Minimum needed by console. We should do a search for calls to env_get()
objects-y+=iwconnect.o pin_map.o console.o microrl.o uart.o helpers.o flash_end_marker.o
objects-y+=cmd_ds18b20.o
objects-y+=cmd_at.o
#objects-y+=cmd_env.o
objects-y+=cmd_iw.o
objects-y+=cmd_common.o hostname.o
subdirs-y+=../frankenstein/src/lwip/core
subdirs-y+=../frankenstein/src/lwip/netif
subdirs-y+=../frankenstein/src/lwip/app
endif

