# Kludgy thing to push our changes over stock OpenWRT tree
#
# TODO: add code to auto git clone etc. later

# This could be a symlink to somewhere else, not tracked in git, or, to a submodule here...

# Targets:
#   build
#   sx1276
#       libsocket
#     mqtt-sn
#     rsmb
#     mosquitto (libmosquito)
#       c-ares

# options: carambola2, raspberrypi
DEVICE?=carambola2
TOP:=$(shell pwd)
TREE=openwrt
CONF=openwrt.config.$(DEVICE)
OWRT_STAGING=$(TOP)/openwrt/staging_dir
include Makefile.$(DEVICE)
BUILDD=build.$(DEVICE)

# FIXME: query root-ar71xx from openwrt/ build
STAGING_DESTDIR=$(OWRT_STAGING)/$(OWRT_TARGET)

MY_CMAKE_OPTS=-DOPENWRT=yes -DCMAKE_TOOLCHAIN_FILE=$(TOP)/toolchain.$(DEVICE).cmake -DCMAKE_INSTALL_PREFIX=/usr
#-DCMAKE_SKIP_RPATH=TRUE

all: prereq build sx1276 rsmb mqtt-sn

# This is annoying because there is no out of tree automake or cmake
# Then again I fixed it by adding one...
$(STAGING_DESTDIR)/usr/bin/broker_mqtts rsmb: $(BUILDD).rsmb/Makefile
	@echo [BUILD] rsmb...
	@cd $(BUILDD).rsmb && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE)
	@cd $(BUILDD).rsmb && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE) install DESTDIR=$(STAGING_DESTDIR)

$(STAGING_DESTDIR)/usr/bin/mqtt-sn-pub $(STAGING_DESTDIR)/usr/bin/mqtt-sn-sub mqtt-sn: $(BUILDD).mqtt-sn/Makefile
	@echo [BUILD] mqtt-sn...
	@cd $(BUILDD).mqtt-sn && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE)
	@cd $(BUILDD).mqtt-sn && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE) install DESTDIR=$(STAGING_DESTDIR)

$(BUILDD).mqtt-sn/Makefile: | ../ext/mqtt-sn-tools
	@echo [PREP] mqtt-sn...
	@mkdir -p $(BUILDD).mqtt-sn
	@cd $(BUILDD).mqtt-sn && export OWRT_STAGING=$(OWRT_STAGING) OWRT_GCC_PFX=$(OWRT_GCC_PFX) OWRT_TARGET=$(OWRT_TARGET) OWRT_TOOLCHAIN=$(OWRT_TOOLCHAIN) && \
											cmake $(MY_CMAKE_OPTS) ../../ext/mqtt-sn-tools

$(BUILDD).rsmb/Makefile: | ../ext/org.eclipse.mosquitto.rsmb
	@echo [PREP] rsmb...
	@mkdir -p $(BUILDD).rsmb
	@cd $(BUILDD).rsmb && export OWRT_STAGING=$(OWRT_STAGING) OWRT_GCC_PFX=$(OWRT_GCC_PFX) OWRT_TARGET=$(OWRT_TARGET) OWRT_TOOLCHAIN=$(OWRT_TOOLCHAIN) && \
											cmake $(MY_CMAKE_OPTS) ../../ext/org.eclipse.mosquitto.rsmb/rsmb/src

sx1276-clean:
	@rm -rf $(BUILDD).sx1276

sx1276: $(STAGING_DESTDIR)/usr/lib/libmosquitto.so $(STAGING_DESTDIR)/usr/lib/libsocket++.so
	@mkdir -p $(BUILDD).sx1276
	@cd $(BUILDD).sx1276 && export OWRT_STAGING=$(OWRT_STAGING) OWRT_GCC_PFX=$(OWRT_GCC_PFX) OWRT_TARGET=$(OWRT_TARGET) OWRT_TOOLCHAIN=$(OWRT_TOOLCHAIN) && \
											cmake $(MY_CMAKE_OPTS) ../../sx1276
	@cd $(BUILDD).sx1276 && STAGING_DIR=$(OWRT_STAGING) $(MAKE)

$(STAGING_DESTDIR)/usr/lib/libsocket++.so libsocket: $(BUILDD).libsocket/Makefile
	@echo [BUILD] libsocket...
	@cd $(BUILDD).libsocket && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE)
	@cd $(BUILDD).libsocket && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE) install DESTDIR=$(STAGING_DESTDIR)

$(BUILDD).libsocket/Makefile: | ../ext/libsocket
	@echo [PREP] libsocket...
	@mkdir -p $(BUILDD).libsocket
	@cd $(BUILDD).libsocket && export OWRT_STAGING=$(OWRT_STAGING) OWRT_GCC_PFX=$(OWRT_GCC_PFX) OWRT_TARGET=$(OWRT_TARGET) OWRT_TOOLCHAIN=$(OWRT_TOOLCHAIN) && \
											cmake $(MY_CMAKE_OPTS) ../../ext/libsocket

$(STAGING_DESTDIR)/usr/lib/libmosquitto.so mosquitto: $(BUILDD).mosquitto/Makefile
	@echo [BUILD] mosquitto...
	@cd $(BUILDD).mosquitto && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE)
	@cd $(BUILDD).mosquitto && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE) install DESTDIR=$(STAGING_DESTDIR)

$(BUILDD).mosquitto/Makefile: $(STAGING_DESTDIR)/usr/lib/libcares.so | ../ext/org.eclipse.mosquitto
	@echo [PREP] mosquitto...
	@mkdir -p $(BUILDD).mosquitto
	@cd $(BUILDD).mosquitto && export OWRT_STAGING=$(OWRT_STAGING) OWRT_GCC_PFX=$(OWRT_GCC_PFX) OWRT_TARGET=$(OWRT_TARGET) OWRT_TOOLCHAIN=$(OWRT_TOOLCHAIN) && \
											cmake $(MY_CMAKE_OPTS) ../../ext/org.eclipse.mosquitto

$(STAGING_DESTDIR)/usr/lib/libcares.so c-ares: $(BUILDD).c-ares/Makefile
	@echo [BUILD] c-ares...
	@cd $(BUILDD).c-ares && export STAGING_DIR=$(OWRT_STAGING) && PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH $(MAKE) install dist DESTDIR=$(STAGING_DESTDIR)

$(BUILDD).c-ares/Makefile: ../ext/c-ares/Makefile.in | ../ext/c-ares
	@echo [PREP] c-ares...
	@mkdir -p $(BUILDD).c-ares
	cd $(BUILDD).c-ares && export OWRT_STAGING=$(OWRT_STAGING) OWRT_GCC_PFX=$(OWRT_GCC_PFX) OWRT_TARGET=$(OWRT_TARGET) OWRT_TOOLCHAIN=$(OWRT_TOOLCHAIN) && \
											PATH=$(OWRT_STAGING)/$(OWRT_TOOLCHAIN)/bin:$$PATH ../../ext/c-ares/configure --host=$(OWRT_TUPLE) --prefix=/usr

../ext/c-ares/Makefile.in: ../ext/c-ares
	@cd ../ext/c-ares && ./buildconf

../ext/org.eclipse.mosquitto:
	(cd ../ext && git clone https://github.com/pastcompute/org.eclipse.mosquitto.git -b sentrifarm )

../ext/c-ares:
	(cd ../ext && git clone https://github.com/bagder/c-ares.git )

../ext/mqtt-sn-tools:
	(cd ../ext && git clone https://github.com/pastcompute/mqtt-sn-tools.git -b sentrifarm )

#../ext/libsocket is presently a .gitsubmodule
#../ext/org.eclipse.mosquitto.rsmb is presently a .gitsubmodule

# libsocket is a submodule
# RSMB is a submodule

prebuild: $(TREE)/feeds.conf $(TREE)/.config patch $(TREE)/tmp/.kconfig-ar71xx_generic

pre2: $(TREE)/tmp/.kconfig-ar71xx_generic

force-config $(TREE)/.config : $(CONF)
	cp -v $(CONF) $(TREE)/.config && cd $(TREE) && make defconfig

build: prebuild
	cd $(TREE) ; make -j4

FEED_PACKAGES = \
			netcat socat nmap wavemon picocom ntpd ntp-utils ntpdate pps-tools i2c-tools \
			file htop less lsof patch coreutils micropython python \
			mjpeg-streamer  nfc-utils irssi-nossl owfs owshell owhttpd owserver luci \
			boost-chrono boost-date_time boost-regex boost-thread boost-timer \
      qt5-core qtserialport


feeds: $(TREE)/feeds.conf
	cd $(TREE) && scripts/feeds update
	cd $(TREE) && scripts/feeds install $(FEED_PACKAGES)

update-feeds $(TREE)/feeds.conf: feeds.conf.in
	sed -e 's|@CUSTOM_FEED_BASE@|$(TOP)/packages|' < feeds.conf.in > $(TREE)/feeds.conf

$(TREE)/tmp/.kconfig-ar71xx_generic: $(TREE)/target/linux/ar71xx/generic/target.mk
	@rm -v $@ ; cd $(TREE) ; make defconfig

patch:
	@for patchfile in `ls patches/ |sort` ; do echo "[PATCH] [$$patchfile]" ;  patch -p1 -N -E -d$(TREE) < patches/$$patchfile ; done || true
	cd $(TREE) ; make defconfig

prereq: | feeds openwrt openwrt/files
	@echo [CHECK] $@

openwrt/files:
	@echo [CHECK] $@
	ln -sf $(PWD)/files $(TREE)/files

openwrt:
	@echo [FETCH] $@
	git clone https://github.com/openwrt-mirror/openwrt.git
	cd openwrt && git checkout -b local-build ab5bd67652b940c8da4229c64eb0cdf09caf4034

.PHONY: all prereq clean build patch feeds prebuild pre2 update-feeds force-config
