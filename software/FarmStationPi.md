# Build instructions : Farm Station

1. Change to the openwrt/ directory
2. Run the following
```
make build DEVICE=raspberrypi
```
The resulting firmware can be found in openwrt/openwrt/bin/bcm2078
3. Create the SD card image

# Assembly instructions

0. This assumes all the components (resistors, capacitors, etc.) have been soldered on to the shield board
1. Insert the inAir9 into the inAir9 shield board
2. Connect the BMP-085 i2c barometer and temperature sensor to the shield board
3. Connect the rain gauge to the spare i2c connector on the shield board
4. Solder a raspberry pi 26 pin stacking adaptor into the adaptor board
5. Connect the shield board to the adaptor board
6. Connect the adaptor board to the rapsberry pi
7. Mount the raspberry pi in the PVC casing
8. Connect the external antenna cable to the inAir9 module
