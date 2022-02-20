# BTcontrol midp client

Midp App für die uralt Nokia Handies. netbeans MIDP Projekt

Hints:
* Nokia 5310 => funktioniert gut (gute Bluetooth Verbindung, Lauter/Leiser Tasten => Geschwindigkeit geht)
* Nokia 3120 classic => funktioniert gut bis auf Lauter/Leiser Tasten

* Startmelodie ausschalten

### Entwicklungsumgebung:

hints für linux:
	Pakete aus 2014: netbeans-8.0-linux.sh sun_java_wireless_toolkit-2_5_2-ml-linux.bin
	java_WTK2.5.2 installieren
	zypper install libXt6-32bit
	wtk/bin/emulator
		javapathtowtk=/home/chris/bin/jdk1.6.0_18/bin/

### jar / jad aufs handy bekommen

installation am nokia handy mit:
```/usr/bin/gammu nokiaaddfile APPLICATION dist/btcontrol```

bsp ~/.config/gammu/config - file:

```
[gammu]
device = 00:11:22:33:44:55
connection = bluephonet
```

oder ussp-push (20161002 hat nicht mehr funktioniert)


### app debuggen

Im emulator funktioniert alles ohne Bluetooth ohne Probleme (dort TCP connect verwenden). Bluetooth Probleme können nur mit 
debugForm.append() geloggt und angeschaut werden. Vor langer Zeit hat der wtk emulator mit Bluetooth einmal funktioniert.
