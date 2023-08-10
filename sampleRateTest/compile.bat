@echo off
avr-gcc -D F_CPU=16000000UL -mmcu=atmega328p -std=c99 -Wall -Os -o main.elf main.c
avr-objcopy -j .text -j .data -O ihex main.elf main.hex
pause
