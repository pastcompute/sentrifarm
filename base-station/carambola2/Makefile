# Kludgy thing to push our changes over stock OpenWRT tree
#
# TODO: add code to auto git clone etc. later

# This could be a symlink to somewhere else, not tracked in git, or, to a submodule here...
TOP:=$(shell pwd)
TREE=openwrt
CONF=openwrt.config
OWRT_TOOLCHAIN=toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2
OWRT_TARGET=target-mips_34kc_uClibc-0.9.33.2
OWRT_GCC_PFX=mips-openwrt-linux-uclibc
OWRT_STAGING=$(TOP)/openwrt/staging_dir
OWRT_TUPLE=mips-openwrt-linux-uclibc

# FIXME: query root-ar71xx from openwrt/ build
STAGING_DESTDIR=$(OWRT_STAGING)/$(OWRT_TARGET)

MY_CMAKE_OPTS=-DOPENWRT=yes -DCMAKE_TOOLCHAIN_FILE=$(TOP)/toolchain.cmake -DCMAKE_INSTALL_PREFIX=/usr
#-DCMAKE_SKIP_RPATH=TRUE

all: prereq build sx1276 rsmb mqtt-sn

# This is annoying because there is no out of tree automake or cmake
# Then again I fixed it by adding one...
$(STAGING_DESTDIR)/usr/bin/broker_mqtts rsmb: build.rsmb/Makefile
	@echo [BUILD] rsmb...
	@cd build.rsmb && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE)
	@cd build.rsmb && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE) install DESTDIR=$(STAGING_DESTDIR)

$(STAGING_DESTDIR)/usr/bin/mqtt-sn-pub $(STAGING_DESTDIR)/usr/bin/mqtt-sn-sub mqtt-sn: build.mqtt-sn/Makefile
	@echo [BUILD] mqtt-sn...
	@cd build.mqtt-sn && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE)
	@cd build.mqtt-sn && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE) install DESTDIR=$(STAGING_DESTDIR)

build.mqtt-sn/Makefile: | ../mqtt-sn-tools
	@echo [PREP] mqtt-sn...
	@mkdir -p build.mqtt-sn
	@cd build.mqtt-sn && export OWRT_STAGING=$(OWRT_STAGING) OWRT_GCC_PFX=$(OWRT_GCC_PFX) OWRT_TARGET=$(OWRT_TARGET) OWRT_TOOLCHAIN=$(OWRT_TOOLCHAIN) && \
											cmake $(MY_CMAKE_OPTS) ../../mqtt-sn-tools

build.rsmb/Makefile: | ../org.eclipse.mosquitto.rsmb
	@echo [PREP] rsmb...
	@mkdir -p build.rsmb
	@cd build.rsmb && export OWRT_STAGING=$(OWRT_STAGING) OWRT_GCC_PFX=$(OWRT_GCC_PFX) OWRT_TARGET=$(OWRT_TARGET) OWRT_TOOLCHAIN=$(OWRT_TOOLCHAIN) && \
											cmake $(MY_CMAKE_OPTS) ../../org.eclipse.mosquitto.rsmb/rsmb/src

sx1276-clean:
	@rm -rf build.sx1276

sx1276: $(STAGING_DESTDIR)/usr/lib/libmosquitto.so $(STAGING_DESTDIR)/usr/lib/libsocket++.so
	@mkdir -p build.sx1276
	@cd build.sx1276 && export OWRT_STAGING=$(OWRT_STAGING) OWRT_GCC_PFX=$(OWRT_GCC_PFX) OWRT_TARGET=$(OWRT_TARGET) OWRT_TOOLCHAIN=$(OWRT_TOOLCHAIN) && \
											cmake $(MY_CMAKE_OPTS) ../../sx1276
	@cd build.sx1276 && STAGING_DIR=$(OWRT_STAGING) $(MAKE)

$(STAGING_DESTDIR)/usr/lib/libsocket++.so libsocket: build.libsocket/Makefile
	@echo [BUILD] libsocket...
	@cd build.libsocket && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE)
	@cd build.libsocket && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE) install DESTDIR=$(STAGING_DESTDIR)

build.libsocket/Makefile: | ../libsocket
	@echo [PREP] libsocket...
	@mkdir -p build.libsocket
	@cd build.libsocket && export OWRT_STAGING=$(OWRT_STAGING) OWRT_GCC_PFX=$(OWRT_GCC_PFX) OWRT_TARGET=$(OWRT_TARGET) OWRT_TOOLCHAIN=$(OWRT_TOOLCHAIN) && \
											cmake $(MY_CMAKE_OPTS) ../../libsocket

$(STAGING_DESTDIR)/usr/lib/libmosquitto.so mosquitto: build.mosquitto/Makefile
	@echo [BUILD] mosquitto...
	@cd build.mosquitto && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE)
	@cd build.mosquitto && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE) install DESTDIR=$(STAGING_DESTDIR)

build.mosquitto/Makefile: $(STAGING_DESTDIR)/usr/lib/libcares.so | ../org.eclipse.mosquitto
	@echo [PREP] mosquitto...
	@mkdir -p build.mosquitto
	@cd build.mosquitto && export OWRT_STAGING=$(OWRT_STAGING) OWRT_GCC_PFX=$(OWRT_GCC_PFX) OWRT_TARGET=$(OWRT_TARGET) OWRT_TOOLCHAIN=$(OWRT_TOOLCHAIN) && \
											cmake $(MY_CMAKE_OPTS) ../../org.eclipse.mosquitto

$(STAGING_DESTDIR)/usr/lib/libcares.so c-ares: build.c-ares/Makefile
	@echo [BUILD] c-ares...
	@cd build.c-ares && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE) install dist DESTDIR=$(STAGING_DESTDIR)

build.c-ares/Makefile: ../c-ares/Makefile.in | ../c-ares
	@echo [PREP] c-ares...
	@mkdir -p build.c-ares
	@cd build.c-ares && export OWRT_STAGING=$(OWRT_STAGING) OWRT_GCC_PFX=$(OWRT_GCC_PFX) OWRT_TARGET=$(OWRT_TARGET) OWRT_TOOLCHAIN=$(OWRT_TOOLCHAIN) && \
											PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH ../../c-ares/configure --host=$(OWRT_TUPLE) --prefix=/usr

../c-ares/Makefile.in:
	@cd ../c-ares && ./buildconf

../org.eclipse.mosquitto:
	(cd .. && git clone git://git.eclipse.org/gitroot/mosquitto/org.eclipse.mosquitto.git )

../c-ares:
	(cd .. && git clone https://github.com/bagder/c-ares.git )


../mqtt-sn-tools:
	(cd .. && git clone git@github.com:pastcompute/mqtt-sn-tools.git )

prebuild: $(TREE)/feeds.conf $(TREE)/.config patch $(TREE)/tmp/.kconfig-ar71xx_generic

pre2: $(TREE)/tmp/.kconfig-ar71xx_generic

$(TREE)/.config : $(CONF)
	cp -v $^ $(TREE)/.config && cd $(TREE) && make defconfig

build: prebuild
	cd $(TREE) ; make -j4

FEED_PACKAGES = \
			netcat socat nmap wavemon picocom ntpd ntp-utils ntpdate pps-tools i2c-tools \
			file htop less lsof patch coreutils micropython python \
			mjpeg-streamer  nfc-utils irssi-nossl owfs owshell owhttpd owserver luci \
			boost-chrono boost-date_time boost-regex boost-thread boost-timer


feeds: $(TREE)/feeds.conf
	cd $(TREE) && scripts/feeds update
	cd $(TREE) && scripts/feeds install $(FEED_PACKAGES)

$(TREE)/feeds.conf: feeds.conf
	@cp -v $^ $@

$(TREE)/tmp/.kconfig-ar71xx_generic: $(TREE)/target/linux/ar71xx/generic/target.mk
	@rm -v $@ ; cd $(TREE) ; make defconfig

patch:
	@for patchfile in `ls patches/ |sort` ; do echo "[PATCH] [$$patchfile]" ;  patch -p1 -N -E -d$(TREE) < patches/$$patchfile ; done || true
	cd $(TREE) ; make defconfig

prereq: | openwrt openwrt/files
	@echo [CHECK] $@

openwrt/files:
	@echo [CHECK] $@
	ln -sf $(PWD)/files $(TREE)/files

openwrt:
	@echo [FETCH] $@
	git clone https://github.com/openwrt-mirror/openwrt.git

.PHONY: all prereq clean build patch feeds prebuild pre2
