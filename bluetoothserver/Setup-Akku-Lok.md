# Akku Lok

Das ist eine Anleitung um eine LGB Lok ohne Schienenstrom fahren lassen zu können. Als Hirn wird ein Raspberry Pi Zero - W verwendet, eine H Brücke am PWM Ausgang, eine USB Soundkarte mit Verstärker und ein paar FETs zum schalten der LEDs. 

Als Raspi Image hab ich DietPi verwendet: https://dietpi.com/downloads/images/DietPi_RPi-ARMv6-Bullseye.7z

### SD Karte in den PC

* DietPi Image auf eine SD-Karte kopieren (Linux: dd_rescue)

a) /boot mounten + /boot/dietpi.text bearbeiten (wifi config) 202202: hat nicht funktioniert
https://dietpi.com/docs/usage/#how-to-do-an-automatic-base-installation-at-first-boot
b) mit usb - ethernet Adapter booten

### Wlan konfigurieren
dietpi-config: (startet automatisch)
Network Options Adapters:
	Wlan ein, key, country-code
	IPv6 off
    Wifi: Auto reconnect on
Network Options Misc: Boot Net Wait: off
Audio Options -> install alsa
Advanced Options:
	swap space weg
	bluetooth on
security options:
	change hostname

### Notwendige Pakete installieren

```
apt-get install git build-essential pkg-config wiringpi libbluetooth-dev libasound2-dev \
libboost-all-dev avahi-daemon
```

Für Entwicklung:
```
apt-get install vim less openssh-client lsof gdb
```

### btcontroll installieren
```
mkdir -p /root/dev/
git clone git@github.com:ferbar/btcontrol  oder mit https://
```

git pager deaktivieren
```
git config --global --replace-all core.pager ""
```

Kompilieren:
```
make
conf Verzeichnis anpassen
make install

./btserver
```

Die Handy / Android App sollte jetzt die Lok finden.

### DietPi für Akku optimieren (readonly filesystem)

```
dietpi-services
disable cron (haut ned hin)
systemctl disable cron
rm -rf /var/lib/dhcp
ln -s /run/ /var/lib/dhcp
ln -s /run /var/lib/run
ln -s /run /var/lib/misc
rm /etc/resolv.conf && ln -s /run/resolv.conf /etc/
mv /var/tmp/ /var/tmp-org/ && ln -s /tmp /var/tmp
```

Anpassungen /lib/systemd/system/systemd-timesyncd.service
```
#PrivateTmp=yes
StateDirectory=run/timesync
```

```
dietpi-drive_manager
```
root und /boot readonly mounten
202203: / ro hat nach einem reboot funktioniert, wenn nicht siehe unten fstab.

Wlan ist nicht schneller _up_ wenn /boot nicht gemountet wird. Problem: die dietpi scripts gehen dann nicht mehr,
bluetooth wird zu früh initialisiert, kein ntp etc... Nur Probleme!
```
reboot
# jetzt sollte der prompt grün sein und (ro) da stehn
rw    # -> disk rw mounten
vi /etc/fstab
```
mit 0 am Ende von / und /boot den fsck on boot disablen

fsck on boot raus aus der config / cmdline.txt => notwendig?


## boot delay auf 0 setzen (default ist 1, viel kanns nicht bringen, von der sd karte abhängig)
```
vi /boot/config.txt
boot_delay=0
```

das sollte mit fstab -> '0' nicht notwendig sein
```
 vi /boot/cmdline.txt
fsck.repair=yes auf fsck.repair=no
```


### DietPi Updates disablen:

vi /boot/dietpi.txt
```
CONFIG_CHECK_DIETPI_UPDATES=0
CONFIG_CHECK_APT_UPDATES=0
```

### vim config tunen (mouse aus)
vi /etc/vim/vimrc.local
```
" Explicitly source defaults.vim so you can override its settings
source $VIMRUNTIME/defaults.vim
" Prevent it from being loaded again later if the user doesn't have
" a vimrc
let skip_defaults_vim = 1
" Disable the settings you don't like
set mouse=
```


```
systemctl mask systemd-rfkill.service
```

wird bei schlechtem empfang mehr ärger machen als es bringt => wifi soft AP weiter unten
```
systemctl disable dietpi-wifi-monitor.service
```

Kopiert logs von /var/tmp nach /var/log ... brauchen wir nicht
```
systemctl disable dietpi-ramlog.service
```

## sound config
=> usb soundkarte wird standardmässig nicht als default genommen
Fehlermeldung:
aplay: set_params:1339: Sample format non available

aplay --dump-hw-params <wavfile>
cat /etc/asound.conf 
defaults.pcm.card 1
defaults.ctl.card 1

## camera config
202203: schon done
/etc/modprobe.d/dietpi-disable_rpi_camera.conf
dietpi-disable_rpi_camera.conf   => blacklist bcm2835_isp

## hdmi output abdrehen
!!!!!!!!!!!!!!!!!!!!!!! TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	=> hdmi-off.service >>>>>>>>>>>>>>>>>>>>>>>>> ins make install tun


```ln -s /run/ /var/lib/bluetooth```


202203: ist weg
-mappt /boot/dietpi ins ram => wozu-
``` systemctl disable vmtouch```


## remove ms repo, don't need on raspi-lok. (installed by raspi-sys-something repo) 202203: ist schon weg
mv /etc/apt/sources.list.d/vscode.list /etc/apt/sources.list.d/vscode.list.disabled

## Setup Wlan soft - AP
apt-get install hostapd dnsmasq lighttpd

Anleitung entsprechend bis zum forward, das brauch ma ned.
https://blog.thewalr.us/2017/09/26/raspberry-pi-zero-w-simultaneous-ap-and-managed-mode-wifi/

dnsmasq.conf:
+ address=/#/192.168.10.1

hostapd.conf: (channel kann irgendwas sein, nimmt den vom verbundenen wlan)
+ multicast_to_unicast=1

lighttpd:

/etc/lighttpd/conf-enabled/redirect.conf
```
   $HTTP["host"] != "raspi-lok" {
        url.redirect = ("" => "http://raspi-lok/")
    }
```

brauch ma nicht
- /sbin/iw phy phy0 interface add ap0 type __ap ; /bin/ip link set ap0 address b8:27:eb:0b:78:f2 ; 
- /bin/ip link set ap0 up; systemctl restart hostapd ; systemctl restart dnsmasq

dietpi-services
=> hostap + dnsmasq systemd controlled machen, exclude from service restart

serial0 abdrehen? => verhindert cpu throttle / stromsparen
	
btcontrol.service:
requires network | bluetooth (!!!! nicht multiuser - wir brauchen keine ntpd zeit !!!!)
systemctl enable btcontrol
