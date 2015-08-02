# Build instructions : Enhanced node

0. Ensure git sumboudles are up to date
1. Change to the openwrt/ directory
2. Run the following
```
make prereq
rm openwrt/.config
make build DEVICE=carambola2
make sx1276 DEVICE=carambola2
```
The resulting firmware can be found in openwrt/openwrt/bin/ar71xx/
3. Upload the firmware as per a normal OpenWRT upgrade.
4. Connect the carambola2 to the network
5. Login with the serial port as root
6. Using scp copy the sx1276 and MQTT software into /root
7. Using scp copy the farmstation script into /etc/init.d
8. Reboot and test

# Assembly instructions

0. This assumes all the components (resistors, capacitors, etc.) have been soldered on
1. Insert the inAir9 into the inAir9 shield board
2. Connect the yRobot i2c RTC into the correct pin header
3. Connect the 1-wire ground temperature sensor
4. Connect the USB webcam cable
5. Connect the USB power cable to a PC
6. Using a terminal program save the date to the RTC. Reboot and check the date is correctly restored
7. Disconnect the PC and screw the Carambola2 assembled module into the PVC case
8. Connect the solar panel USB output to the carambola2 USB power
9. Connect the external antenna cable to the inAir9 module
10. Connect the wifi button
