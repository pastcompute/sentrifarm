# OPENWRT_GCC=openwrt/.../bin/mips-openwrt-linux-uclibc-gcc cmake -DCMAKE_TOOLCHAIN_FILE=this/file ..

SET(CMAKE_SYSTEM_NAME Linux)

set(OPENWRT_TOOLCHAIN_DIR "$ENV{OWRT_STAGING}/$ENV{OWRT_TOOLCHAIN}")
set(OPENWRT_TARGET_DIR "$ENV{OWRT_STAGING}/$ENV{OWRT_TARGET}")
SET(CMAKE_C_COMPILER "${OPENWRT_TOOLCHAIN_DIR}/bin/$ENV{OWRT_GCC_PFX}-gcc")
SET(CMAKE_CXX_COMPILER "${OPENWRT_TOOLCHAIN_DIR}/bin/$ENV{OWRT_GCC_PFX}-g++")

SET(CMAKE_PREFIX_PATH "${OPENWRT_TARGET_DIR}/usr")
SET(CMAKE_FIND_ROOT_PATH ${OPENWRT_TARGET_DIR} ${OPENWRT_TOOLCHAIN_DIR})
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)

set(ENV{PKG_CONFIG_PATH} ${OPENWRT_TARGET_DIR}/usr/lib/pkgconfig)
message(STATUS "PATH=$ENV{PATH}")
message(STATUS "PKG_CONFIG_PATH=$ENV{PKG_CONFIG_PATH}")

# Notes:
# http://cpprocks.com/using-cmake-to-build-a-cross-platform-project-with-a-boost-dependency/
# http://stackoverflow.com/questions/17721619/cmake-cross-compile-libraries-are-not-found
# https://github.com/mysmartgrid/OpenWRT_SDK/blob/master/Toolchain-OpenWRT.cmake
