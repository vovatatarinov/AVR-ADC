@echo off
avrdude -p m328p -b 115200 -P COM4 -c arduino -e -U flash:w:main.hex
pause
