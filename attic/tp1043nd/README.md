# This directory holds configuration for building a custom OpenWRT for using a TPLink TP1043ND router for remote data logging.

openwrt.config          OpenWRT configuration file, built against OpenWRT CC beta hash 8dac80b7028dded770d063a036af21a51b540d72
kernel.patch            OpenWRT kernel patch to enable RTC over i2c for the TPLink Tp1043ND GPIO hardware mod

# Instructions

```
git clone git://git.openwrt.org/openwrt.git
cd openwrt
scripts/feeds update
scripts/feeds install netcat picocom
cp ../openwrt.config openwrt/.config
make defconfig
patch -p1 < ../kernel.patch
make
# Go have a long coffee break
```

For flashing instructions refer to the OpenWRT wiki.

To complete replace the firmware and configuration on a router already running OpenWRT, if you have a local web server:

sysupgrade -n http://path/to/openwrt/bin/ar71xx/openwrt-ar71xx-generic-tl-wr1043nd-v1-squashfs-factory.bin

# Configuration

* Configured to uplink to my house net on the wan port and act as a WIFI access point for data logging
* Enable SSH login from my local network on the WAN side
* Enable web access from my local network on the WAN side
* Update NTP server to my local network 
* Install logging script into init.d
* Setup USB vfat mount to USB stick -- done manually via logging script
