# TODO

Sentrifarm has been developed in our spare time, time that is quite limited due to our day jobs, including running a farm, and families.

Thus there are a lot of things that we would like to do to turn the prototype into a fully running system that still need to be done.

Regardless of the hackaday prize outcome, we intend to have a running system by the end of the year so that we can start collecting and analysing real data in a meaningful way.

## Build several more nodes

Although the modular pluggable design allows us to test all the modules, only two nodes are presently deployed (one station and one remote), due to time and funding contraints. More radios were purchased, this time the 20dBm modules, so we replaced the 14dBm modules with those, they likely hepled with our great range results.

We intend to modify the Station unit design so that one station can communicate with two remotes using one radio and antenna, however this depends on having time to upgrade the software from a simple point to point protocol to use the proper LoRa protocol.

## Deploy more sensors

The first prototypes only had a few sensors. The critical path is really expanding the range (*Done*, see below), improving robustness of communications (*Done* using properly designed antennas and fixes to the messaging have allow us to be 99% reliable when properly aligned and in range) and understanding the power usage (will take a lot more analysis and experiment).

More sensors can be added relatively easily later. Presently supported: temperature, barometer, humidity, UV, daylight (via solar panel voltage), and wind (not fully tested due to equipment failure)

## investigate interoperability

We have been provided with a different radio module to evaluate and would like to see how it works communicating with our existing kit

## Extend deployment range

~~So far we have spent little time on understanding and tuning the radio links and antenna.~~
The inAir9 LoRa maximum design range exceeds 15km, so significant future effort will be extended on achieving the desired range.

We succeeded in what we thought was spectacular fashion, see project logs [Great weather for Fox Hunting](https://hackaday.io/project/4758/log/25075-great-weather-for-fox-hunting-at-last) and [Up towers and through scrub](https://hackaday.io/project/4758/log/25150-up-towers-and-through-scrub])

## Improve solar power efficiency

We havent had much time to spend on characterising and improving the power usage and solar efficieny as yet. This needs to be done.

## Consolidate and shrink the circuit designs

Because we only had limited funding, I designed a set of circuit boards that allow us to make nodes in a wide variety of configurations (and computing components) for testing whilst only needing two radios, and being able to maximise the gear we had on hand.

The next step is to evaluate what we have learned and build more production oriented, smaller and dedicated circuits.

## Produce an Enhanced node using a more widely available / smaller compute module

See above

## Implement NFC

For the low power standard nodes, add an NFC module so that the farmer can tap to get recent sensor readings

## Improved microcontroller build

Presently I have been using platformio to get moving quickly for building the software portably over the Teensy and the ESP201. However I have realised that platformio is too opaque in how it discovers and downloads software, so I am intending to migrate to Cmake and a custom build script. I'll may look at moving away from arduino to native for the ESP modules as well.

## Things we hope to have TODO'ne by end of August

* Build two more nodes, using breadboard even if needed
* Relay of barometer, temperature, humidity and UV data from a nodes, via the farm station to ~~the OpenHAB system~~ Grafana *DONE* smoke, rain not done
* Screenshots of ~~OpenHAB output~~ Grafana dashboards *DONE*
* Operating at a distance of a few hundred metres  *VERY DONE*

## Goals for end of September

* Extend RF hop range out to 5km.  Antennas & radio tuning! *DONE!*

## Next steps

* Unfinished items above, in particular, consoldiating all the hacks made to the circuitry and making a new board
* Improve reliability issues
* Finish implementing wind, and add live GFDI calculation to the dashboard
* Finish the rain gauge
