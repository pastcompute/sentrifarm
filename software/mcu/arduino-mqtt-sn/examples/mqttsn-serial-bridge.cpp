/*
mqttsn-serial-bridge.cpp

The MIT License (MIT)

Copyright (C) 2014 John Donovan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <Arduino.h>

#include "mqttsn.h"
#include "mqttsn-messages.h"

#include <JeeLib.h>
#include <Ports.h>

#define ADVERTISE_INTERVAL 10
#define RF_CHANNEL 111

uint8_t response_buffer[RF12_MAXDATA];
msg_advertise adv;
const uint8_t node_id = 1;

void serialEvent() {
    if (Serial.available() > 0) {
        uint8_t* response = response_buffer;
        uint8_t packet_length = (uint8_t)Serial.read();
        uint8_t plength_temp = packet_length;
        *response++ = plength_temp--;

        while (plength_temp > 0) {
            while (Serial.available() > 0) {
                *response++ = (uint8_t)Serial.read();
                --plength_temp;
            }
        }

        while (!rf12_canSend()) {
            rf12_recvDone();
        }
        rf12_sendStart(0, response_buffer, packet_length);
        rf12_sendWait(2);
    }
}

void setup() {
    Serial.begin(115200);

    adv.length = sizeof(msg_advertise);
    adv.type = ADVERTISE;
    adv.gw_id = node_id;
    adv.duration = ADVERTISE_INTERVAL;

    rf12_initialize(node_id, RF12_868MHZ, RF_CHANNEL);
}

int main() {
    init();

    setup();

    rf12_sleep(RF12_WAKEUP);

    bool running = true;
    uint32_t cur_millis = 0;
    uint32_t prev_millis = 0;

    while (running) {
		if (serialEventRun) {
		    serialEventRun();
		}

        if (rf12_recvDone()) {
            if (rf12_crc == 0) {
                Serial.write((const uint8_t*)rf12_data, rf12_data[0]);
                Serial.flush();
            }
        } else {
            // Send the ADVERTISE message roughly every ADVERTISE_INTERVAL seconds. This also handles the 50-day 32-bit
            // unsigned integer overflow, but it will send a few messages more frequently at overflow because the
            // maximum value of a uint32 is not a multiple of (ADVERTISE_INTERVAL * 1000). Practically this shouldn't
            // cause any issues.
            prev_millis = cur_millis;
            cur_millis = millis() % (ADVERTISE_INTERVAL * 1000L);

            if (cur_millis < prev_millis) {
                rf12_sendStart(0, &adv, adv.length);
                rf12_sendWait(2);
            }
        }
    }

    rf12_sleep(RF12_SLEEP);

    return 0;
}
