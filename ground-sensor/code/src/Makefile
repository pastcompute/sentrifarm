objects-y+=sentrifarm.o
objects-y+=ds18b20.o tcp_push.o

ifneq ($(SENTRI_STANDALONE),y)
subdirs-y+=../frankenstein/src/
else
objects-y+=env.o iwconnect.o pin_map.o console.o microrl.o entry.o uart.o helpers.o flash_end_marker.o
objects-y+=cmd_ds18b20.o
objects-y+=cmd_at.o
objects-y+=cmd_env.o
objects-y+=cmd_iw.o
objects-y+=cmd_common.o hostname.o
subdirs-y+=../frankenstein/src/lwip/core
subdirs-y+=../frankenstein/src/lwip/netif
subdirs-y+=../frankenstein/src/lwip/app
endif

