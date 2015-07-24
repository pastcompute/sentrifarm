win32 {
    HOMEDIR += $$(USERPROFILE)
}
else {
    HOMEDIR += $$(PWD)
}

TEMPLATE = app
CONFIG -= qt

PLATFORMIO = $(HOME)/.platformio

INCLUDEPATH += "$${PLATFORMIO}/packages/framework-arduinoespressif/variants/generic"
INCLUDEPATH += "$${PLATFORMIO}/packages/framework-arduinoespressif/cores/esp8266"
INCLUDEPATH += "$${PLATFORMIO}/packages/framework-arduinoespressif/cores/esp8266/spiffs"
INCLUDEPATH += "$${PLATFORMIO}/packages/framework-arduinoespressif/libraries/SPI"
INCLUDEPATH += "$${PLATFORMIO}/packages/toolchain-xtensa/xtensa-lx106-elf/include"
INCLUDEPATH += "$${PLATFORMIO}/packages/toolchain-xtensa/lib/gcc/xtensa-lx106-elf/4.8.2/include"
INCLUDEPATH += "$${PLATFORMIO}/packages/toolchain-xtensa/lib/gcc/xtensa-lx106-elf/4.8.2/include"
INCLUDEPATH += "$${PLATFORMIO}/packages/sdk-esp8266/include"
INCLUDEPATH += "./lib/SX1276lib"
INCLUDEPATH += "./lib/arduino-mqtt-sn"
INCLUDEPATH += "./lib/Adafruit_BMP085_Unified"
INCLUDEPATH += "./lib/Adafruit_Sensor"

win32:INCLUDEPATH ~= s,/,\\,g

DEFINES += "F_CPU=80000000L"
DEFINES += "__ets__"
DEFINES += "ICACHE_FLASH"
DEFINES += "ARDUINO_ESP8266_ESP01"
DEFINES += "ARDUINO_ARCH_ESP8266"
DEFINES += "ESP8266"
DEFINES += "ARDUINO=10601"

OTHER_FILES = \
    ./lib/arduino-mqtt-sn/examples/mqttsn-serial-bridge.cpp \
    platformio.ini

SOURCES += \
    ./src/node.ino \
    ./src/sx1276mqttsn.cpp \
    ./lib/SX1276lib/sx1276.cpp \
    ./lib/arduino-mqtt-sn/mqttsn-messages.cpp

HEADERS += \
    ./src/sx1276mqttsn.h \
    ./lib/SX1276lib/sx1276.h \
    ./lib/SX1276lib/sx1276reg.h \
    ./lib/arduino-mqtt-sn/mqttsn.h \
    ./lib/arduino-mqtt-sn/mqttsn-messages.h

message("CONFIG=$$CONFIG")
