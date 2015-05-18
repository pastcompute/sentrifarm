#!/bin/bash

TROOT=openwrt/build_dir/target-mips_34kc_uClibc-0.9.33.2/linux-ar71xx_generic/
SROOT=openwrt/target/linux/ar71xx/files

#FILES=package/kernel/linux/modules/other.mk package/kernel/w1-gpio-custom/src/w1-gpio-custom.c
# target/linux/ar71xx/config-3.18 

FILES="arch/mips/ath79/dev-m25p80.c arch/mips/ath79/mach-carambola2.c"

diff -u openwrt/package/kernel/w1-gpio-custom/src/w1-gpio-custom.c $TROOT/w1-gpio-custom

for x in $FILES ; do
  diff -u $SROOT/$x $TROOT/linux-3.18.11/$x
done

