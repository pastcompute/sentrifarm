#  Copyright (c) 2015 Andrew McDonnell <bugs@andrewmcdonnell.net>
#
#  This file is part of SentriFarm Radio Relay.
#
#  SentriFarm Radio Relay is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  SentriFarm Radio Relay is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with SentriFarm Radio Relay.  If not, see <http://www.gnu.org/licenses/>.

# This software uses the following third party libraries, listed here with associated licenses:
#
# * mosquitto : Eclipse Distribution License (EDL) 1.0
# * c-ares (used by mosquitto) MIT License, portions open source ISC license
# * boost : Boost Software License 1.0
#
# Before adding new libraries, reread https://wiki.52north.org/bin/view/Documentation/ThirdPartyLicensesForGPL

cmake_minimum_required(VERSION 2.8.11)   # <-- target_include_directories() needs 2.8.11
project(sx1276 C CXX)

#message("CMAKE_HOST_SYSTEM=${CMAKE_HOST_SYSTEM}")
#message("CMAKE_SYSTEM=${CMAKE_SYSTEM} CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
#message("CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME} CMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION} CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}")

if(CMAKE_COMPILER_IS_GNUCC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -pthread")
endif(CMAKE_COMPILER_IS_GNUCC)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_XOPEN_SOURCE=600")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_XOPEN_SOURCE=600")
set (CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIBDIR}")

find_package(Threads)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED OFF)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.49.0 COMPONENTS system chrono thread)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

# It would be handy if all I needed to do was: find_package(Mosquitto REQUIRED)
# Ref: http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries
# Once done this will define
#  MOSQUITTO_FOUND
#  MOSQUITTO_INCLUDE_DIRS
#  MOSQUITTO_LIBRARIES
#  MOSQUITTO_DEFINITIONS
#
# Mosquitto needs c-ares:
#
#  apt-get install libc-ares-dev  # https://github.com/bagder/c-ares.git
#
# Mosquitto needs libuuid:
#
#
# Local testing example:
#
# Prerequisite:
#
#   cd base-station
#   git clone git://git.eclipse.org/gitroot/mosquitto/org.eclipse.mosquitto.git
#   ( cd org.eclipse.mosquitto && make -j DESTDIR=$HOME/opt/mosquitto-1.4.2 prefix= install )
#
# Cross-compile testing example:
#
#   { set environment as required ... } cmake -DCMAKE_TOOLCHAIN_FILE=<...> -DMOSQUITTO_HINT_x86=...path/to/xcc/mosquitto

set(MOSQUITTO_HINT_x86 $ENV{HOME}/opt/mosquitto-1.4.2)

find_path(MOSQUITTO_INCLUDE_DIR mosquitto.h HINTS ${MOSQUITTO_HINT_x86}/include REQUIRED)
find_library(MOSQUITTO_LIBRARY NAMES mosquitto HINTS ${MOSQUITTO_HINT_x86}/lib REQUIRED)
set(MOSQUITTO_LIBRARIES ${MOSQUITTO_LIBRARY} )
set(MOSQUITTO_INCLUDE_DIRS ${MOSQUITTO_INCLUDE_DIR} )
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Mosquitto DEFAULT_MSG MOSQUITTO_LIBRARY MOSQUITTO_INCLUDE_DIR)
mark_as_advanced(MOSQUITTO_INCLUDE_DIR MOSQUITTO_LIBRARY )

# For whatever reason we dont automagically find the dependencies of mosquitto.
# Possibly this is because I turned off rpath or am doing something else wrong, so fix it here
find_package(PkgConfig REQUIRED)

option(enable_maintainer "Enable maintainer CFLAGS (-Wall -Werror)" OFF)
if(enable_maintainer)
if(CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
endif(CMAKE_COMPILER_IS_GNUCXX)
endif(enable_maintainer)

# FIXME This should probably be a lib, sort it out later
set(MY_FILES buspirate_binary.c buspirate_spi.cpp sx1276_platform.cpp misc.cpp spidev_spi.cpp sx1276.cpp spi.hpp util.hpp)
set(MY_LIBS ${CMAKE_THREAD_LIBS_INIT} ${Boost_SYSTEM_LIBRARY} ${Boost_CHRONO_LIBRARY})

add_executable(bp_sx1276_dump bp_sx1276_dump.c buspirate_binary.c )
add_executable(sx1276_test1_tx sx1276_test1_tx.cpp ${MY_FILES})
add_executable(sx1276_test1_rx sx1276_test1_rx.cpp ${MY_FILES})

add_executable(test_mqtt_discard test_mqtt_discard.cpp)     # dumb version using C'ish C++ and mosquito client library
add_executable(test_mqtt_discard2 mqttclient.cpp test_mqtt_discard2.cpp)   # discard test using C++ MQTT class

target_include_directories(test_mqtt_discard PRIVATE ${MOSQUITTO_INCLUDE_DIR})
target_include_directories(test_mqtt_discard2 PRIVATE ${MOSQUITTO_INCLUDE_DIR})

target_link_libraries(sx1276_test1_tx ${MY_LIBS})
target_link_libraries(sx1276_test1_rx ${MY_LIBS})
target_link_libraries(test_mqtt_discard ${MY_LIBS} ${MOSQUITTO_LIBRARIES})
target_link_libraries(test_mqtt_discard2 ${MY_LIBS} ${MOSQUITTO_LIBRARIES})
