# TODO

Sentrifarm has been developed in our spare time, time that is quite limited due to our day jobs, including running a farm, and families.

Thus there are a lot of things that we would like to do to turn the prototype into a fully running system that still need to be done.

Regardless of the hackaday prize outcome, we intend to have a running system by the end of the year so that we can start collecting and analysing real data in a meaningful way.

## Build several more nodes

Although the modular pluggable design allows us to test all the modules, only two nodes are presently deployed (one station and one remote), due to time and funding contraints. Two more modules are being purchased to allow us to deploy two legs concurrently.

We intend to modify the Station unit design so that one station can communicate with two remotes using one radio and antenna, however this depends on having time to upgrade the software from a simple point to point protocol to use the proper LoRa protocol.

## Deploy more sensors

The first prototypes only have a few sensors. The critical path is really expanding the range, improving robustness of communications and understanding the power usage.

More sensors can be added relatively easily later.

## Extend deployment range

So far we have spent little time on understanding and tuning the radio links and antenna.
The inAir9 LoRa maximum design range exceeds 15km, so significant future effort will be extended on achieving the desired range.

## Improve solar power efficiency

We havent had much time to spend on characterising and improving the power usage and solar efficieny as yet. This needs to be done.

## Consolidate and shrink the circuit designs

Because we only had limited funding, I designed a set of circuit boards that allow us to make nodes in a wide variety of configurations (and computing components) for testing whilst only needing two radios, and being able to maximise the gear we had on hand.

The next step is to evaluate what we have learned and build more production oriented, smaller and dedicated circuits.

## Produce an Enhanced node using a more widely available / smaller compute module

See above

## Improved microcontroller build

Presently I have been using platformio to get moving quickly for building the software portably over the Teensy and the ESP201
However I have realised that platformio is too opaque in how it discovers and downloads software, so I am intending to migrate to
Cmake and a custom build script. I'll probably look at moving away from arduino tp native for the ESP modules as well.

## Things we hope to have TODO'ne by August 17

* Relay of barometer, temperature, smoke, rain and UV data from a nodes, via the farm station to the OpenHAB system,
* Screenshots of OpenHAB output
* Photos of a protoype with a solar panel and sensors in a real paddock
* Operating at a distance of a few hundred metres

