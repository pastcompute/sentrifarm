# OPENWRT_GCC=openwrt/.../bin/mips-openwrt-linux-uclibc-gcc cmake -DCMAKE_TOOLCHAIN_FILE=this/file ..

SET(CMAKE_SYSTEM_NAME Linux)

set(OPENWRT_TOOLCHAIN_DIR "$ENV{OWRT_STAGING}/$ENV{OWRT_TOOLCHAIN}")
set(OPENWRT_TARGET_DIR "$ENV{OWRT_STAGING}/$ENV{OWRT_TARGET}")
SET(CMAKE_C_COMPILER "${OPENWRT_TOOLCHAIN_DIR}/bin/$ENV{OWRT_GCC}")

SET(CMAKE_FIND_ROOT_PATH ${OPENWRT_TARGET_DIR} ${OPENWRT_TOOLCHAIN_DIR})

# Notes:
# http://cpprocks.com/using-cmake-to-build-a-cross-platform-project-with-a-boost-dependency/
# http://stackoverflow.com/questions/17721619/cmake-cross-compile-libraries-are-not-found
# https://github.com/mysmartgrid/OpenWRT_SDK/blob/master/Toolchain-OpenWRT.cmake
