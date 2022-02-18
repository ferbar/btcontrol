#!/bin/bash
# ???
# pio device monitor -p /dev/tty.SLAB_USBtoUART -b 921600 -f time -f colorize -f log2file -f esp32_exception_decoder
if [ -c /dev/ttyUSB0 ] ; then
	stty -F /dev/ttyUSB0 115200 -icanon -echo time 0 min 0 -echoe -echok -echoctl -echoke -iexten -icrnl
	tail -f /dev/ttyUSB0
else
	echo "=========== /dev/ttyUSB1 =========="
	stty -F /dev/ttyUSB1 115200 -icanon -echo time 0 min 0 -echoe -echok -echoctl -echoke -iexten -icrnl
	tail -f /dev/ttyUSB1
fi
