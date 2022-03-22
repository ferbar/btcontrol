#!/bin/bash
# ???
# pio device monitor -p /dev/tty.SLAB_USBtoUART -b 921600 -f time -f colorize -f log2file -f esp32_exception_decoder
function logtail() {
	port=$1
	stty -F $port 115200 -icanon -echo time 0 min 0 -echoe -echok -echoctl -echoke -iexten -icrnl
	tail -f $port | xargs -IL date +"%Y%m%d_%H%M%S:L"
}

if [ "$1" ] ; then
	if [ -c $1 ] ; then
		logtail $1
	else
		echo "$1 doesn't exist"
	fi
else

	if [ -c /dev/ttyUSB0 ] ; then
		logtail /dev/ttyUSB0
	else
		echo "=========== /dev/ttyUSB1 =========="
		logtail /dev/ttyUSB1
	fi
fi
