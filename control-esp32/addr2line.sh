#!/bin/bash

if [ -z "$1" ] ; then
	echo "$1 \"Backtrace:0x400e263a:0x3ffdb8600x400d59ff:0x3ffdb900 0x400d8446:0x3ffdb930 ....\""
	exit 1
fi

bt=${1#Backtrace:}
bt=${bt// /\\n}

echo -e "$bt" | ~/.platformio/packages/toolchain-xtensa32/bin/xtensa-esp32-elf-addr2line -e ./.pio/build/wemos_d1_mini32/firmware.elf
