"""
    Build script for openwrt_mips_carambola2.py
    openwrt_mips_carambola2-builder.py
"""

from os.path import join
from SCons.Script import AlwaysBuild, Builder, Default, DefaultEnvironment


# Copy this file into $PLATFORMIO_HOME_DIR/platforms (home_dir)

env = DefaultEnvironment()

# hack hack hack

# A full list with the available variables
# http://www.scons.org/doc/production/HTML/scons-user.html#app-variables
env.Replace(
    AR="mips-openwrt-linux-ar",
    AS="mips-openwrt-linux-as",
    CC="mips-openwrt-linux-gcc",
    CXX="mips-openwrt-linux-g++",
    OBJCOPY="mips-openwrt-linux-objcopy",
    RANLIB="mips-openwrt-linux-ranlib",
    SIZETOOL="mips-openwrt-linux-size"
)

env.PrependENVPath(
    "PATH",
    "/home/andrew/develop/k5/sentrifarm/software/openwrt/openwrt/staging_dir/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/bin"
)

env['ENV']['STAGING_DIR'] = "/home/andrew/develop/k5/sentrifarm/software/openwrt/openwrt/staging_dir"


#
# Target: Build executable and linkable firmware
#
target_bin = env.BuildProgram() # BuildFirmware()

#
# Target: Define targets
#
Default(target_bin)

