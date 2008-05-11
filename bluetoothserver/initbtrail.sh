#!/bin/sh
# hcitool hci0 piscan
# dbus-send --system --type=method_call --print-reply --dest=org.bluez /org/bluez/hci0 org.bluez.Adapter.SetMode string:discoverable
# sdptool browse ff:ff:ff:00:00:00 -> port registrierungen anzeigen
# port 30 als SerialProfile registrieren
sdptool add --channel=30 SP
sdptool setattr 0x10005 0x100 "btrail"

# port 30 auf /dev... mappen
# rfcomm -r listen /dev/rfcomm30 30
