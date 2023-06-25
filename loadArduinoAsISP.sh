#!/bin/sh
avrdude -p m328p -b 19200 -P /dev/ttyACM0 -c stk500v1 -e -U flash:w:main.hex
