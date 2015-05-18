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

add_executable(bp_sx1276_dump bp_sx1276_dump.c buspirate_binary.c )
add_executable(bp_sx1276_test1 bp_sx1276_test1.cpp buspirate_binary.c buspirate_spi.cpp spi_factory.cpp misc.cpp spidev_spi.cpp sx1276.cpp spi.hpp)
target_link_libraries(bp_sx1276_test1 ${CMAKE_THREAD_LIBS_INIT} ${Boost_SYSTEM_LIBRARY} ${Boost_CHRONO_LIBRARY})

