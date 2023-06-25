#!/bin/sh
avrdude -p m328p -b 115200 -P /dev/ttyACM0 -c arduino -e -U flash:w:main.hex
