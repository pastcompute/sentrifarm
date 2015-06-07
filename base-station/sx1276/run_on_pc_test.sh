#!/bin/bash
#
# Run up RSMB MQTT-SN broker and relay using bus pirate
#

RSMB_DIR=${PWD}/../org.eclipse.mosquitto.rsmb/rsmb/src

xterm -title "Broker" -fn 6x9 -geom 96x8+200+100 -hold -e ${RSMB_DIR}/broker_mqtts rsmb-config-pc-test&

