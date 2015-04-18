#
# Using esp8266-frankenstein as a base.
# -------------------------------------
#
# We have two choices:
# (a) patch src/main.c to call our code, and src/Makefile to add our modules
# (b) just replace src/main.c and src/Makefile with our own. That way we have total control
#     of the entire binary and we are essentially only using Antares as a build system
#
# The potential downside of (b) is divergence from upstream, and/or we lose the console mode.
# Now strictly for our sensors, we dont really need the console mode: the thing wakes up, 
# measures stuff, connects & transmits it then goes back to sleep
# However it has its uses, including debugging and testing, and facilitating contributions back
# upstream.
#
# Maybe we can have our cake and eat it too: structure our entry point in sentrifarm.c such that
# it can be called both from a patched src/main.c and our own user_main() function.
#
# So thus we can build two firmwares:
# (1) A 'production' firmware with only our code, essentially a bare-metal ESP8266 binary
#     although it will be leveraging bits of frankenstein such as LWIP
#
# (2) A diagnostics firmware that includes the console.
#
# We switch between this by building with SENTRI_STANDALONE=n (if unspecified, defaults to y, and you dont get any of frankenstein)
# In STANDALONE mode though, you get no TCP/IP telnet server (even if enabled in the CONFIG), and almost
# no commands ( for the moment, we leave ds18b20 and iw for diagnostics)
#
# On a related note, it may be possible to retask the serial lines of the ESP-01 (esp. RX)
# to gain additional GPIO without soldering. Especially if we never need to type any commands in...
#


# TODO: add a rule to fetch the submodules if missing
# TODO: add a rule to fetch the esp open sdk submodule if missing

FRANK=frankenstein
FRANK_CONFIG=config-esp8266
SDK=esp-open-sdk
COLOURDIFF=$(shell which colordiff || echo diff 2> /dev/null)
PORT?=/dev/ttyUSB0

# If you want to build with the frankenstein console included, use
# make distclean ; make SENTRI_STANDALONE=n
# This will generate a debugging firmware

SENTRI_STANDALONE?=y

# Set this in your environment as needed
KCONFIG_FRONTEND?=/opt/kconfig-frontends/bin

HERE=$(PWD)

ifeq ($(V),1)
define FRANK_VERBOSE
CONFIG_BUILD_VERBOSE=y
endef
endif

define FRANK_PATH
export PATH=$(PWD)/$(SDK)/xtensa-lx106-elf/bin:$(KCONFIG_FRONTEND):$$PATH ;
endef

all: build

# Method #1 : build over the top of a frankenstein so we have extended diagnostic ability
#
# 1. Apply patch that enables the diagnostic firmware:
#    Add hook to user_main in src/main.c
# 2. Build by changing project_sources=../src and have it link back to frankenstein
#    Note this puts our .o files in frankenstein/build/app and doesnt pollute the source tree
#
# Method #2 : build using frankenstein / antares infrastructure, but essentially we replace
# all of frankenstein except for a few necessary bits. To do it that way requires us replacing
# frankenstein/src/Makefile, or even, directly using antares here
#
# Here we achieve this by examining SENTRI_STANDALONE and building symlinked versions of necessary source files
#
# Ideally we would do both firmwares from one execution of make, but this gets a bit fiddly, because
# we cant specify the 'build' directory to use so there will be object file clobbering
# So presently you need to do a make distclean between changing over.
# Unfortunately, build/ is hardcoded inside antares/make/Makefile.lib
#
build: prereq $(FRANK_CONFIG)
	@patch -p1 -N -r- -s -d frankenstein < frankenstein.patch  || true
	$(FRANK_PATH) $(MAKE) -C $(FRANK) $(FRANK_VERBOSE) project_sources="../src" SENTRI_STANDALONE=$(SENTRI_STANDALONE)

distclean:
	$(FRANK_PATH) $(MAKE) -C $(FRANK) distclean

clean:
	$(FRANK_PATH) $(MAKE) -C $(FRANK) clean

menuconfig:
ifeq ($(FORCE),)
	@diff -q $(FRANK)/.config $(FRANK_CONFIG) || { echo "[WARNING] Config in $(FRANK) is different from $(FRANK_CONFIG). Fix that first." ; $(MAKE) diffconfig ; false ; }
endif
	$(FRANK_PATH) $(MAKE) -C $(FRANK) menuconfig
	@diff -q $(FRANK)/.config $(FRANK_CONFIG) || cp -v $(FRANK)/.config $(FRANK_CONFIG)

forceconfig:
	@cp -v $(FRANK_CONFIG) $(FRANK)/.config

diffconfig:
	@$(COLOURDIFF) $(FRANK)/.config $(FRANK_CONFIG)

prereq: | $(KCONFIG_FRONTEND) $(SDK) $(FRANK) $(FRANK)/.config $(FRANK)/antares .stamp.frank

# I would rather use a symlink, but make menuconfig keeps breaking it
$(FRANK)/.config:
	@cp -v $(FRANK_CONFIG) $(FRANK)/.config

# We track frankenstein as a submodule because then we can stick at a specific git version
# So this should be a NO-OP
$(FRANK):
	@echo "oops - $(FRANK) is missing, this should be a git submodule!"
	@false

# Similarly, antares...
$(FRANK)/antares: $(FRANK)
	$(FRANK_PATH) $(MAKE) -C $(FRANK) antares GITURL=https://github.com/pastcompute/antares.git

$(KCONFIG_FRONTEND):
	@echo "oops - $@ is missing
	@echo "Clone and build http://ymorin.is-a-geek.org/git/kconfig-frontends/"
	@false

# On my workstation I symlink this elsewhere so I dont have to keep building it
# For a fresh build on a new PC we want to clone it from github
# Dont bother using a submodule (although perhaps that might be useful, so it can be stuck at a particular version)
$(SDK):
	@echo "oops - $@ is missing, either create a symlink, or clone it here!"
	@echo "Clone alternatives:"
	@echo "1. https://github.com/pastcompute/esp-open-sdk"
	@echo "2. https://github.com/pfalcon/esp-open-sdk"
	@false

# Force a distclean in frankenstein when first setup
.stamp.frank:
	$(MAKE) -C $(FRANK) distclean
	touch $@

# Shortcut flash instead of using antares deploy which forces a whole build
flash:
	-$(FRANK_PATH) esptool.py --port $(PORT) write_flash 0x00000 $(FRANK)/images/antares.rom

.PHONY: all build prereq forceconfig menuconfig distclean clean flash

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`
# Notes
#
# make CONFIG_BUILD_VERBOSE=y will build verbose, although you can also put this into .config
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

