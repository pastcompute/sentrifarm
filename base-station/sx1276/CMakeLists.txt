cmake_minimum_required(VERSION 2.8)
project(sx1276 C CXX)
set(CMAKE_C_FLAGS "-std=c99 -pthread")

find_package(Threads)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED OFF)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.49.0 COMPONENTS system chrono thread)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

# This would be handy: find_package(Mosquitto REQUIRED)
# TODO: move to a .cmake file
# Ref: http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries
# Once done this will define
#  MOSQUITTO_FOUND
#  MOSQUITTO_INCLUDE_DIRS
#  MOSQUITTO_LIBRARIES
#  MOSQUITTO_DEFINITIONS
# TODO : ref sources dir in hints in case mosquitto cloned relative to sx1276/
find_path(MOSQUITTO_INCLUDE_DIR mosquitto.h HINTS ${HOME}/org.eclipse.mosquitto/lib REQUIRED)
find_library(MOSQUITTO_LIBRARY NAMES mosquitto HINTS ${HOME}/org.eclipse.mosquitto/lib REQUIRED)
set(MOSQUITTO_LIBRARIES ${MOSQUITTO_LIBRARY} )
set(MOSQUITTO_INCLUDE_DIRS ${MOSQUITTO_INCLUDE_DIR} )
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Mosquitto DEFAULT_MSG MOSQUITTO_LIBRARY MOSQUITTO_INCLUDE_DIR)
mark_as_advanced(MOSQUITTO_INCLUDE_DIR MOSQUITTO_LIBRARY )

if(CMAKE_COMPILER_IS_GNUCC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif(CMAKE_COMPILER_IS_GNUCC)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

option(enable_maintainer "Enable maintainer CFLAGS (-Wall -Werror)" OFF)
if(enable_maintainer)
if(CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
endif(CMAKE_COMPILER_IS_GNUCXX)
endif(enable_maintainer)

# FIXME This should probably be a lib, sort it out later

set(MY_FILES buspirate_binary.c buspirate_spi.cpp sx1276_platform.cpp misc.cpp spidev_spi.cpp sx1276.cpp spi.hpp)

add_executable(bp_sx1276_dump bp_sx1276_dump.c buspirate_binary.c )
add_executable(sx1276_test1_tx sx1276_test1_tx.cpp ${MY_FILES})
add_executable(sx1276_test1_rx sx1276_test1_rx.cpp ${MY_FILES})

add_executable(test_mqtt_discard test_mqtt_discard.cpp)

target_link_libraries(sx1276_test1_tx ${CMAKE_THREAD_LIBS_INIT} ${Boost_SYSTEM_LIBRARY} ${Boost_CHRONO_LIBRARY})
target_link_libraries(sx1276_test1_rx ${CMAKE_THREAD_LIBS_INIT} ${Boost_SYSTEM_LIBRARY} ${Boost_CHRONO_LIBRARY})
target_link_libraries(test_mqtt_discard ${CMAKE_THREAD_LIBS_INIT} ${Boost_SYSTEM_LIBRARY} ${Boost_CHRONO_LIBRARY} ${MOSQUITTO_LIBRARIES})

