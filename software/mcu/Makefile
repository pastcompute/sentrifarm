# For the time being, I am sticking with platformio for building the MCU software
# Ideally I'll convert it to cmake or something else, but I dont have time to 
# sort through the specific idiosyncracies of that for the moment
# and I'd rather just make a working v1 sentrifarm system.
# Perfection is the enemy of good and all that...

# Prerequisites are listed in platformio.txt
platformio:;
	cd sx1276-beacon && platformio run
	cd sx1276-rx && platformio run

.PHONY: platformio
