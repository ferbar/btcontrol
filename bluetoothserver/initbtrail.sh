#!/bin/sh
# TODO: seit bluez 5 ist sdptool default mässig abgedreht,
# https://github.com/pauloborges/bluez/blob/master/doc/profile-api.txt
# http://docs.huihoo.com/qt/qtextended/4.4/bluetooth-bluetoothservice.html
# http://www.bluez.org/bluez-5-api-introduction-and-porting-guide/
#     RegisterProfile on the ProfileManager1 interface
#
# hint: kernel bug "Can't init device hci0: Operation not supported (95)" ab 3.10
# hciconfig sagt "down" hci0 up sagt das oben -> anderen dt dongle nehmen | anderen kernel nehmen
# 
# sdptool browse ff:ff:ff:00:00:00 -> port registrierungen anzeigen
# port 30 als SerialProfile registrieren
SDPTOOL="sudo sdptool"
HCICONFIG="sudo hciconfig"

CONFFILE="$(dirname $0)/conf/initbtrail.conf"
if [ -f $CONFFILE ] ; then
	. $CONFFILE
fi

if [ "$1" = "--wait-for-device" ] ; then
	while ! $HCICONFIG | grep -q "UP RUNNING" ; do
		echo "waiting for bluetooth dongle";
		sleep 1;
	done
fi

if $HCICONFIG hci0 ; then
	:
else
	echo "[error] no hci0 bluetooth stick detected" >&2
	exit 0
fi

if $SDPTOOL browse ff:ff:ff:00:00:00 | grep -q btrail ; then
	echo "[ok] sdp service schon registriert"
else
	if $SDPTOOL add --channel=30 SP && $SDPTOOL setattr 0x10005 0x100 "btrail" ; then
		echo "[ok] sdp registered"
	else
		echo "[error] error registering SDP" >&2
		exit 1
	fi
fi


# port 30 auf /dev... mappen
# rfcomm -r listen /dev/rfcomm30 30

# wenn pc master kann er bis zu 7 connections machen
$HCICONFIG hci0 lm master

# verbindungen anzeigen
# hcitool con
# verbindungsqualität anzeigen
# watch hcitool rssi 00:12:EE:21:20:63

# checken ob piscan eingeschalten ist:
#
# in /var/lib/bluetooth/<blue_id>/config eingetragen habe 
# mode discoverable 
# discovto 0 
sleep 1
$HCICONFIG | grep ISCAN -q
rc=$?
if [ $rc != 0 ]; then
	echo "bluetooth ISCAN noch nicht eingeschalten"
	if $HCICONFIG hci0 piscan ; then
		echo "[ok] ... eingeschalten"
	else
		echo "[error] '$HCICONFIG hci0 piscan'" >&2
	fi
# dbus-send --system --type=method_call --print-reply --dest=org.bluez /org/bluez/hci0 org.bluez.Adapter.SetMode string:discoverable
else
	echo "[ok] ISCAN eingeschalten"
fi
