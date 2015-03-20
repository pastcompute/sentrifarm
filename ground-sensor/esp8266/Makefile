SRCDIR=antares
GITURL=https://nekromant@github.com/pastcompute/antares.git
BRANCH=experimental
OBJDIR=.
TMPDIR=tmp
TOPDIR=.
project_sources=src
ANTARES_DIR:=./antares
ANTARES_INSTALL_DIR:=$(abspath ./antares)

CFLAGS+=-I$(abspath ./include/lwip-esp8266/)
CFLAGS+=-D__ets__ \
	-DICACHE_FLASH \
	-DLWIP_OPEN_SRC \
	-DPBUF_RSV_FOR_WLAN \
	-DEBUF_LWIP

PORT?=/dev/ttyUSB0

HAVE_PL2303?=y


ifeq ($(ANTARES_INSTALL_DIR2),)
antares:
	git clone $(GITURL) $(ANTARES_DIR) -b$(BRANCH)
	@echo "I have fetched the antares sources for you to $(ANTARES_DIR)"
	@echo "Please, re-run make"
else
antares:
	ln -sf $(ANTARES_INSTALL_DIR) $(ANTARES_DIR)
	@echo "Using global antares installation: $(ANTARES_INSTALL_DIR)"
	@echo "Symlinking done, please re-run make"
endif

-include Makefile.local-settings
-include antares/Makefile

# You can define any custom make rules right here!
# They will can be later hooked as default make target
# in menuconfig 

ifeq ($(ANTARES_INSTALL_DIR2),)

else
# Hook ensure_config so make works when I deleted .config
ifeq ($(strip $(CONFIG_MAKE_DEFTARGET)),)
.PHONY: my_ensure_config
.DEFAULT_GOAL := my_ensure_config
my_ensure_config: .config
	$(MAKE)

.config:
	cp my_defconfig .config
endif
endif

#GP0 - Loader GPIO
#GP1 - reset
#Press both.
#release reset.
#sleep 300 ms
#release boot
ifeq ($(HAVE_PL2303),y)
define tobootloader
	sudo pl2303gpio --gpio=0 --out=0 --gpio=1 --out=0 \
	--gpio=1 --in --sleep 50 \
	--gpio=0 --in
endef 

define reset
	sudo pl2303gpio --gpio=1 --out=0 --gpio=1 --in
endef
endif

ifeq ($(HAVE_PL2303),y)
define minicom
	minicom -o -D $(PORT) -b 115200
endef
endif

dumpiram:
	$(tobootloader)
	esptool.py --port $(PORT) dump_mem 0x40000000 65536 iram0.bin

reset: 
	$(reset)

flash:
	$(tobootloader)
	-esptool.py --port $(PORT) write_flash 0x00000 images/antares.rom
	$(reset)
	$(minicom)

flashidata:
	$(tobootloader)
	-esptool.py --port $(PORT) write_flash 0x7c000 esp_iot_sdk_v0.9.2/bin/esp_init_data_default.bin
	$(reset)
	$(minicom)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`
# Notes
#
# make CONFIG_BUILD_VERBOSE=y will build verbose, although you can also put this into .config
# THis will show the compiler command lines
#
#
# Config option ESP8266_BLOB_LWIP will use proprietary lwip instead of our open source version
# This seems to make the bare image 6kb smaller 191356 -> 185616
#
# Config option CONFIG_ESP8266_BLOB_SSL can be turned off in the minimum, not sure when it gets used
#
# CONFIG_CC_OPT2 instead of CONFIG_CC_OPTSZ doesnt make a difference?! (Adds 7kb in frankenstein, so obviously code dependent)
#
# CONFIG_GCC_PARANOID_WRN --> Wall
# CONFIG_GCC_STRIP --> doesnt work (breaks link)
# CONFIG_GCC_LM=y  use -lm - seems to be safely removable

