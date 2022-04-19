# Bluetooth Server
🚂🚃🚃🚃

## Akku Lok
Eine Einsatzmöglichkeit ist ein Raspberry PI Zero W mit einer H-Brücke und Mosfets zur Ansteuerung für Lichter.
<img src="img_akku_lok.jpg" alt="Akkulok" height="250" alt="RhB 182"/>

Als H-Brücke kann z.b. ein L6203 oder ein VNH5019A verwendet werden. FETs für Lampen: IRFML8244. Statt dem 
Raspberry PI PWM0 (GPIO18) kann auch ein digispark (Attiny85) verwendet werden. Sound kann über einen I2S DAC (MAX98357) oder eine USB Soundkarte und
Verstärker generiert werden.

Setup mit Raspbian (DietPI) siehe [Setup-Akku-Lok.md](Setup-Akku-Lok.md)

## Handy Steuerung für DCC Loks
Mit SRCPD kann ein DCC Signal an eine Serielle Schnittstelle gelegt werden welches dann von einem Booster
(z.B. von MERG) für die Schienen verstärkt wird. Kann mehrere Loks auf einem Gleis steuern.

Setup siehe Setup-SRCPD.md

cpp-programm, zum kompilieren wird benötigt:
* suse 11.2: libusb-dev bzw libusb-compat-devel, bluez-devel
* auf ubuntu: libbluetooth-dev, libusb-dev, libasound-dev, libboost-dev
* am raspi: libusb-1.0-0-dev libbluetooth-dev libasound2-dev libboost-serialization-dev | für raspi pwm: wiringpi (seit jessie als paket, davor: git clone git://git.drogon.net/wiringPi)

  im server.cpp ist die Adresse vom SRCPD hardcoded auf 127.0.0.1

## Merg - selbstbau - Bosster über Serielle Schnittstelle

[NB1A_partlist.txt](../NB1A_partlist.txt)
Partlist für den http://merg.co.uk - booster


## Handy Steuerung für Analog Loks:
Velleman k8055: uralt Variante um PWM direkt auf die Schienen zu legen (für analog Loks)
* k8055 git submodule downloaden:
```
git submodule update --init
```

*********************** F9: MotorBoost


## Wichtige Programme:

### bluetoothserver
Der eigentliche Daemon Prozess, kompiliertes C++ Programm. 

### bluetoothserver/initbtrail.sh
registriert das serial-profile Service damit das Handy weiss dass auf channel 30 der btserver rennt

kurze Bluetooth Einführung: damit ein Service gefunden wird muss es zuerst mit sdptool registriert werden,damit der PC überhaupt gefunden wird muss PISCAN eingeschalten sein.
Für Bluez 5 muss der bluetoothd mit --compat gestartet werden. Das muss im /etc/systemd/system/dbus-org.bluez.service extra hinzugefügt werden.

## Konfiguration
Die .sample files unter bluetoothserver/conf können als Ausgangsbasis verwendet werden.

### bluetoothserver/conf/lokdef.csv
Wenn der SRCPD verwendet wird müssen hier sämtliche Lokomotiven eingetraen werden. Bei einer Akku Lok darf nur eine Lokomotive eingetragen sein.

CSV Datei mit:

1. Spalte: Adresse
2. Decoder Typ
3. Name
4. Bildchen
5. Anzahl Funktionen

Der Rest: funktionsnamen


### bluetoothserver starten:
```
./btserver --help liefert eine mini-hilfe
```

oder das init-script nach /etc/init.d kopieren und einschalten. **hint**: möchte man bluetooth zum steuern verwenden dann bei Required-Start: bluetooth: hinzufügen!

Neu:
```
systemctl enable btcontrol
```



## Setup auf OpenSuse & SRCPD



