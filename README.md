# btcontroll 

Modelleisenbahn übers Handy/Smartphone drahtlos steuern!
Entweder mit einem Raspberry Pi in der Lok selbst (das ist nur für die Gartenbahn interessant) oder über SRCPD und einen Booster eine DCC Lok ansterern. Bei Midp (die alten Nokias) verwende ich Bluetooth, für Android ist derzeit nur Wlan als Kommunikationsmedium eingebaut.

Diese Software ist meine private Spielerei, es sind jedoch alle herzlich willkommen den Code zu begutachten, auszuprobieren und Ideen beizutragen. Sollte allerdings irgendwas beschädigt werden ist jeder selbst schuld und ich lehne jegliche Verantwortung ab.


## bluetoothserver
Vermittlungsstelle bluetooth -> srcpd
cpp-programm, zum kompilieren wird benötigt:
* suse 11.2: libusb-dev bzw libusb-compat-devel, bluez-devel
* auf ubuntu: libbluetooth-dev, libusb-dev, libasound-dev, libboost-dev
* am raspi: (für raspi pwm): wiringPi git clone git://git.drogon.net/wiringPi
* k8055 git submodule downloaden:
```
git submodule update --init
```

  im server.cpp ist die Adresse vom SRCPD hardcoded auf 127.0.0.1

### bluetoothserver/initbtrail.sh
registriert das serial-profile Service damit das Handy weiss dass auf channel 30 der btserver rennt

kurze Bluetooth Einführung: damit ein Service gefunden wird muss es zuerst mit sdptool registriert werden,damit der PC überhaupt gefunden wird muss PISCAN eingeschalten sein

### bluetoothserver/lokdef.csv
cvs datei mit:

1. spalte: adresse
2. decoder typ
3. name
4. bildchen
5. anzahl funktionen

rest: funktionsnamen

### bluetoothserver starten:
```
./btserver --help liefert eine mini-hilfe
```
oder das init-script nach /etc/init.d kopieren und einschalten. **hint** möchte man bluetooth zum steuern verwenden dann bei Required-Start: bluetooth: hinzufügen!
```
update-rc.d btcontrol enable
```

### Schnittstellen zum Motor:

* Velleman k8055: uralt Variante um PWM direkt auf die Schienen zu legen
* SRCPD: eine Software die über den Com Port DCC Commands an einen Booster schicken kann. Kann viele Loks auf einmal steuern.
* Raspberry Pi + Digispark: kleine Attiny85 Platine und eine H - Brücke
* Raspberry Pi ohne extra MC: H-Brücke hängt direkt am Raspberry Pi / PWM0 (hier ist leider kein Sound möglich da PWM + Sound die selbe Hardware verwenden)

## control-android

Android App
siehe control-android/README.txt

## control-midp
das MIDP - java Programm welches aufs Handy gehört

die App ist unter /dist/btcontroll.jar

### midptestenv
Emulator um MIDP Programme am PC rennen zu lassen (zum Testen ganz nett, hat aber bugs)

## NB1A_partlist.txt
partlist für den merg.co.uk - booster

## usb k8055
lib um eine Viessman - platine anzusteuern, früher für PWM verwendet (ohne DCC)

## ussp-push-0.11
ussp-push -> ussp-push-0.11/src/ussp-push
obex - push programm, programm um dateien über bluetooth an ein handy zu senden
ussp-push 00:11:22:33:44:55@ btcontrol.jar btcontrol.jar

## Raspberry PI

### Dateisystem readonly
warum? damit man ohne schlechtes Gewissen den Stecker ziehen kann. Vorallem die boot Partition (FAT) kann leicht beleidigt werden. (dass man dann erst nach mount -o remount,rw / und /boot was änern kann versteht sich von selbst)

```
 update-rc.d rsyslog disable
 sudo vi /etc/fstab
   options: defaults,ro
```

wenn man das nicht machen will und es reicht einem nach dem ersten superblock time is in future: /etc/e2fsck.conf und broken_system_clock reinschreiben. Siehe:
http://unix.stackexchange.com/questions/8409/how-can-i-avoid-run-fsck-manually-messages-while-allowing-experimenting-with-s

### Hostname ändern
/etc/hosts und /ets/hostname anpassen

dhcp & avahi übernimmt den hostname

bluetooth [4.99] speichert unter /var/lib/bluetooth/<mac>/config den hostname. Kann dort und per 
 dbus-send --print-reply --system --dest=org.bluez /org/bluez/$(pidof bluetoothd)/hci0 org.bluez.Adapter.SetProperty string:'Name' variant:string:'<neuer name>'
geändert werden
