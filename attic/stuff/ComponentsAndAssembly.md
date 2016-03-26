# System Components

At time of submission there are several Sentrifarm node prototypes.

* Station
* Standard remote node 
* Standard remote node with wifi
* Enhanced remote node

## Farm Station

The farm station consists of a RaspberryPi, V1 sx1276 shield and V1 adaptor shield.
As this is based at the farm it can be powered with a micro USB mains power adaptor.
This is connected via ethernet to the farmhouse network.
The farm station receives data from the radio via MQTT-SN and relays via MQTT to an OpenHAB installation on the farm network.
The first prototype farm station also has its own sensors:
* Air temperature / Barometer
* Rain gauge
The farm station also makes a rolling recording of all messages to a USB stick as a backup

See [software/FarmStationPi.md] for building the OpenWRT SD card for the Raspberry Pi
See [hardware/FarmStationPi.md] for assembling the farm statiom

## Enhanced Node

The enhanced node consists of a Carambola2, and a V1 sx1276 shield.
Being linux based it can support USB devices but requires more power and thus larger solar panels and batteries.
This first prototype has the following sensors:
* Ground temperature
* Web cam used to take delay series of the crop for later analysis. Images are stored locally on a USB stick for later 
* Battery backed RTC module for timestamping images
pickup by wifi or USB, only the act of taking an image is sent via MQTT
This enhanced node also provides a Wifi access point, that the farmer can press a button to turn on,
then connect phone web browser to view all information
The first prototype only runs during daylight hours as we need to improve the solar efficiency of the system

See [software/EnhancedNodeCarambola2.md] for building and flashing the OpenWRT image
See [hardware/EnhancedNodeCarambola2.md] for assembling the enhanced node

## Standard node with Wifi

The standard node with wifi consists of a ESP201 module, V1 sx1276 shield and V1 adaptor shield.
The standard node should run logging all night.
This first prototype has the following sensors:
* Air temperature / barometer
* Ground temperature
* Wind

See [software/ESP201.md] for building and flashing the OpenWRT image
See [hardware/ESP201.md] for assembling the standard node with wifi

## Standard node

The standard node with wifi consists of a TEENSY-LC module, V1 sx1276 shield and V1 adaptor shield.
The standard node should run logging all night.
This first prototype has the following sensors:
* Air temperature / barometer
* Ground temperature
* UV sensor
* Buffet sensor - records how much buffeting the module itself is receiving due to wind
* Smoke / gas  ** using an MQ2

See [software/TEENSYLC.md] for building and flashing the OpenWRT image
See [hardware/TEENSYLC.md] for assembling the standard node

## Enhanced Rain Gauge Module

The enhanced rain gauge module will have an ATtiny85 for recording rain flow and reporting the information to any of the nodes.

See [software/RainGauge.md] for building and flashing the OpenWRT image
See [hardware/RainGauge.md] for assembling the rain gauge module

