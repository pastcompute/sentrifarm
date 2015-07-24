win32 {
    HOMEDIR += $$(USERPROFILE)
}
else {
    HOMEDIR += $$(PWD)
}

# I think just getting the following settings correct is enough for #include file and code completion
# (as in, setting an ESP8266 kit (or not) doesnt make a difference

TEMPLATE = app
CONFIG -= qt

PLATFORMIO = $(HOME)/.platformio

INCLUDEPATH += "$${PLATFORMIO}/packages/framework-arduinoespressif/variants/generic"
INCLUDEPATH += "$${PLATFORMIO}/packages/framework-arduinoespressif/cores/esp8266"
INCLUDEPATH += "$${PLATFORMIO}/packages/framework-arduinoespressif/cores/esp8266/spiffs"
INCLUDEPATH += "./src"
INCLUDEPATH += "$${PLATFORMIO}/packages/framework-arduinoespressif/libraries/SPI"
INCLUDEPATH += "$${PLATFORMIO}/packages/toolchain-xtensa/xtensa-lx106-elf/include"
INCLUDEPATH += "$${PLATFORMIO}/packages/toolchain-xtensa/lib/gcc/xtensa-lx106-elf/4.8.2/include"

win32:INCLUDEPATH ~= s,/,\\,g

DEFINES += "F_CPU=80000000L"
DEFINES += "__ets__"
DEFINES += "ICACHE_FLASH"
DEFINES += "ARDUINO_ESP8266_ESP01"
DEFINES += "ARDUINO_ARCH_ESP8266"
DEFINES += "ESP8266"
DEFINES += "ARDUINO=10601"

OTHER_FILES += \
    platformio.ini

SOURCES += \
    ./src/dump.ino

message("CONFIG=$$CONFIG")
