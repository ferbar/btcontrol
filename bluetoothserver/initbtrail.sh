#!/bin/sh
# hcitool hci0 piscan
# dbus-send --system --type=method_call --print-reply --dest=org.bluez /org/bluez/hci0 org.bluez.Adapter.SetMode string:discoverable
# sdptool browse ff:ff:ff:00:00:00 -> port registrierungen anzeigen
# port 30 als SerialProfile registrieren
sdptool add --channel=30 SP
sdptool setattr 0x10005 0x100 "btrail"

# port 30 auf /dev... mappen
# rfcomm -r listen /dev/rfcomm30 30

# wenn pc master kann er bis zu 7 connections machen
/usr/sbin/hciconfig hci0 lm master

# verbindungen anzeigen
# hcitool con
# verbindungsqualität anzeigen
# watch hcitool rssi 00:12:EE:21:20:63

# checken ob piscan eingeschalten ist:
#
# in /var/lib/bluetooth/<blue_id>/config eingetragen habe 
# mode discoverable 
# discovto 0 
/usr/sbin/hciconfig hci0 | grep ISCAN -q
rc=$?
if [ $rc != 0 ]; then
	echo "bluetooth ISCAN nicht eingeschalten!" > /dev/stderr
fi
