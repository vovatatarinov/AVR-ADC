#!/bin/sh
avr-gcc -D F_CPU=16000000UL -mmcu=atmega328p -Wall -Os -o main.elf main.c
avr-objcopy -j .text -j .data -O ihex main.elf main.hex
