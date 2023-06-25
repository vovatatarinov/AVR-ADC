@echo off
avrdude -p m328p -b 19200 -P COM4 -c stk500v1 -e -U flash:w:main.hex
pause
