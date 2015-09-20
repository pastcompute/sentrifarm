# Farmer Stay In bed?

This is the main git repository for our entry into the 2015 hackaday prize.
For background see [the Sentrifarm project page on hackaday.io](http://hackaday.io/project/4758).

## Hackaday Prize Entry Qualifications

* [Project discussion](https://hackaday.io/project/4758)
* 2-minute [Quarter Final Submission Video](https://www.youtube.com/watch?v=L6NtuzwXCsY)
* Artist [impression](media/ArtisticImpression.png)
* [Project logs](https://hackaday.io/project/4758)
* [System design diagram](media/architecturediagramv0.2.svg)
* [System design - operational concept](OperationalConcept.md)
* [System design - component assembly](ComponentsAndAssembly.md)
* [TODO List](TODO.md)

## Licenses

* Hardware designs released under Creative Commons CC-BY-SA.
* Source code released under GPL v3 except where existing imported components are otherwise identified as being used under other licenses such as MIT, BSD, etc.
* Clipart in diagrams obtained from open / CC license sources : open office gallery, openclipart.com
* Our own photos and diagrams and videos under CC-BY-NC
* We reserve the right to dual-license software that we developed under alternative licenses

### Third party software components incorporated into the repository

* mqttsn-messages.cpp - MIT license

### Third party software components incorporated into the repository as a submodule

* Adafruit arduino sensor library - Apache license
* Adafruit BMP085 library - open source unspecified
* Arduino MQTTSN library - MIT license
* Arduino DHT11 library - MIT license
* Frankenstein & Antares - GPL licensed ESP8266 framework. Note, not actively used at present tie but this may change.

### Third party software components submodules or cloned and binaries cross-compiled during the build

* openwrt - linux distribution - distribution is GPL, components are their respective open source licenses
* c-ares - library used by mosquitto; MIT license
* mosquitto - MQTT client tools and library; used under Eclipse Distribution License (not the EPL)
* mqtt-sn-tools - MQTT-SN client tools; BSD-like license
* RSMB - MQTT and MQTT-SN broker; Eclipse Distribution License
* libsocket++ - C++ socket library; BSD license

The following third party software components are then dynamically linked to sentrifarm software:

libmosquitto - dynamically linked, thus used as allowed under the terms of the EDLv1.0

