# Procedure for testing

0. Latest software on carambola2 and teensy or esp201
1. On the carambola2, open 3 ssh windows
2. Run each of the following commands in a window

```
./sx1276_mqttsn_bridge /dev/spidev0.1 1885
```

```
broker_mqtts rsmb-config-carambola2-test 
```

```
mqtt-sn-sub -t '#' -v
```

After booting the ESP201 after a little while, every 10 seconds there should be a topic message on the third console.

```
sentrifarm/node/beacon: 1f 01 4094 1913846737 1.0.1
```

which is just the ESP8266 versioning and a tick count.
For some reason my board is reporting 4094 mV for VCC which is totally wrong!
