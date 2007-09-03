#!/bin/sh
# sdptool browse ff:ff:ff:00:00:00 -> port registrierungen anzeigen
# port 30 als SerialProfile registrieren
sdptool add --channel=30 SP
sdptool setattr 0x10005 0x100 "btrail"

# port 30 auf /dev... mappen
# rfcomm -r listen /dev/rfcomm30 30
