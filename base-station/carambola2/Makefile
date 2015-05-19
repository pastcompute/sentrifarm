# Kludgy thing to push our changes over stock OpenWRT tree
#
# TODO: add code to auto git clone etc. later

# This could be a symlink to somewhere else, not tracked in git, or, to a submodule here...
TREE=openwrt
CONF=openwrt.config
OWRT_TOOLCHAIN=toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2
OWRT_TARGET=target-mips_34kc_uClibc-0.9.33.2
OWRT_GCC=mips-openwrt-linux-uclibc-gcc
OWRT_STAGING=$(shell pwd)/openwrt/staging_dir

all: prereq build

sx1276-clean:
	@rm -rf build.sx1276

sx1276:
	@mkdir -p build.sx1276
	@cd build.sx1276 && export OWRT_STAGING=$(OWRT_STAGING) OWRT_GCC=$(OWRT_GCC) OWRT_TARGET=$(OWRT_TARGET) OWRT_TOOLCHAIN=$(OWRT_TOOLCHAIN) && \
											cmake -DCMAKE_TOOLCHAIN_FILE=../carambola2/toolchain.cmake ../../sx1276
	@cd build.sx1276 && export STAGING_DIR=$(OWRT_STAGING) && make

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
