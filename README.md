# btcontroll 

SRCPD über bluetooth mit einem MIDP handy steuern

Diese Software ist meine private Spielerei, es sind jedoch alle herzlich willkommen den code zu begutachteni, auszuprobieren und Ideen beizutragen.
Sollte allerdings irgendwas beschädigt werden ist jeder selbst schuld und ich lehne jegliche Verantwortung ab.


## bluetoothserver
Vermittlungsstelle bluetooth -> srcpd
cpp-programm, zum kompilieren wird benötigt:
  suse 11.2: libusb-dev bzw libusb-compat-devel, bluez-devel
  auf ubuntu: libbluetooth-dev, libusb-dev, libasound-dev, libboost-dev
  am raspi: (für raspi pwm): wiringPi git clone git://git.drogon.net/wiringPi
  k8055 git submodule downloaden:
  	git submodule update --init

  im server.cpp ist die Adresse vom SRCPD hardcoded auf 127.0.0.1

### bluetoothserver/initbtrail.sh
registriert das serial-profile service damit das handy weiss dass auf channel 30 der btserver rennt
kurze bluetooth einführung: damit ein service gefunden wird muss es zuerst mit sdptool registriert werden,
  damit der PC überhaupt gefunden wird muss PISCAN eingeschalten sein

### bluetoothserver/lokdef.csv
cvs datei mit:
1. spalte: adresse
2. decoder typ
3. name
4. bildchen
5. anzahl funktionen
rest: funktionsnamen

### bluetoothserver starten:
./btserver --help liefert eine mini-hilfe

## control-android

Android App

## control-midp
das MIDP - java Programm welches aufs Handy gehört
MIDP client
btcontroll.jar -> MidpBluetoothExample2/MobileApplication/dist/btcontroll.jar


### midptestenv
emulator um MIDP programme am PC rennen zu lassen (zum testen ganz nett, hat aber bugs)

## NB1A_partlist.txt
partlist für den merg.co.uk - booster

## usb k8055
lib um eine Viessman - platine anzusteuern, früher für PWM verwendet (ohne DCC)

## ussp-push-0.11
ussp-push -> ussp-push-0.11/src/ussp-push
obex - push programm, programm um dateien über bluetooth an ein handy zu senden
ussp-push 00:11:22:33:44:55@ btcontrol.jar btcontrol.jar

## Raspberry PI Dateisystem readonly
warum? damit man ohne schlechtes Gewissen den Stecker ziehen kann. Vorallem die boot Partition (FAT) kann leicht beleidigt werden. (dass man dann erst nach mount -o remount,rw / und /boot was änern kann versteht sich von selbst)

 update-rc.d rsyslog disable
 sudo vi /etc/fstab
   options: defaults,ro

wenn man das nicht machen will und es reicht einem nach dem ersten superblock time is in future: /etc/e2fsck.conf und broken_system_clock reinschreiben. Siehe:
http://unix.stackexchange.com/questions/8409/how-can-i-avoid-run-fsck-manually-messages-while-allowing-experimenting-with-s
