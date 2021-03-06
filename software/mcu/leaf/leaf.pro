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
INCLUDEPATH += "$${PLATFORMIO}/packages/framework-arduinoespressif/libraries/Wire"
INCLUDEPATH += "$${PLATFORMIO}/packages/toolchain-xtensa/xtensa-lx106-elf/include"
INCLUDEPATH += "$${PLATFORMIO}/packages/toolchain-xtensa/lib/gcc/xtensa-lx106-elf/4.8.2/include"
INCLUDEPATH += "$${PLATFORMIO}/packages/toolchain-xtensa/lib/gcc/xtensa-lx106-elf/4.8.2/include"
INCLUDEPATH += "$${PLATFORMIO}/packages/sdk-esp8266/include"
INCLUDEPATH += "./lib/DHT-sensor-library"
INCLUDEPATH += "./lib/SX1276lib"
INCLUDEPATH += "./lib/arduino-mqtt-sn"
INCLUDEPATH += "./lib/Adafruit_BMP085_Unified"
INCLUDEPATH += "./lib/Adafruit_Sensor"
INCLUDEPATH += "./lib/sentrifarm"

DEFINES += "F_CPU=80000000L"
DEFINES += "__ets__"
DEFINES += "ICACHE_FLASH"
DEFINES += "ARDUINO_ESP8266_ESP01"
DEFINES += "ARDUINO_ARCH_ESP8266"
DEFINES += "ESP8266"
DEFINES += "ARDUINO=10604"
DEFINES += "PLATFORMIO=020202"

OTHER_FILES += \
    platformio.ini

SOURCES += \
    ./src/leaf.ino \
    ./lib/SX1276lib/sx1276.cpp \
    ./lib/sentrifarm/sx1276mqttsn.cpp \
    ./lib/sentrifarm/sf-mcu.cpp \
    ./lib/sentrifarm/sf-sensordata.cpp \
    ./lib/DHT-sensor-library/DHT.cpp \
    ./lib/arduino-mqtt-sn/mqttsn-messages.cpp

HEADERS += \
    ./lib/sentrifarm/sf-ioadaptorshield.h \
    ./lib/sentrifarm/sf-mcu.h \
    ./lib/SX1276lib/sx1276.h \
    ./lib/SX1276lib/sx1276reg.h \
    ./lib/sentrifarm/sx1276mqttsn.h \
    ./lib/arduino-mqtt-sn/mqttsn.h \
    ./lib/arduino-mqtt-sn/mqttsn-messages.h \
    ./lib/DHT-sensor-library/DHT.h \
    lib/sentrifarm/sf-sensordata.h \
    lib/sentrifarm/sf-util.h \
    lib/sentrifarm/sf-bmp180.h \
    lib/sentrifarm/sf-ds1307.h \
    lib/sentrifarm/sf-pcf8591.h

