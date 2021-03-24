## Setup mit DietPi (2021.02)

Das ist die Anleitung um ein komplett neues DietPi - Image für eine neue Rasperry PI Akku Lok zu erstellen:

### DietPi image auf eine SD-Karte kopieren (Linux: dd_rescue)

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
dietpi-services:
disable cron (haut ned hin)
systemctl disable cron
rm -rf /var/lib/dhcp
ln -s /run/ /var/lib/dhcp
ln -s /run /var/lib/run
rm /etc/resolv.conf && ln -s /run/resolv.conf /etc/
mv /var/tmp/ /var/tmp-org/ && ln -s /tmp /var/tmp
```

Anpassungen /lib/systemd/system/systemd-timesyncd.service
```
#PrivateTmp=yes
StateDirectory=run/timesync
```

dietpi-drive_manager:
root und /boot readonly mounten
/ muss in der fstab auf ro gestellt werden: (/boot sollte schon RO sein)
```
vi /etc/fstab
```

!!!!!!! nicht vergessen: mit 0 am Ende von / und /boot den fsck on boot disablen !!!!!!
fsck on boot raus aus der config / cmdline.txt

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

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! sound config !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
=> usb soundkarte wird standardmässig nicht als default genommen
Fehlermeldung:
aplay: set_params:1339: Sample format non available

aplay --dump-hw-params <wavfile>
cat /etc/asound.conf 
defaults.pcm.card 1
defaults.ctl.card 1


dietpi-disable_rpi_camera.conf   => blacklist bcm2835_isp

hdmi output abdrehen => hdmi-off.service >>>>>>>>>>>>>>>>>>>>>>>>> ins make install tun


btcontrol.service:
requires network | bluetooth (!!!! nicht multiuser - wir brauchen keine ntpd zeit !!!!)

ln -s /run/ /var/lib/bluetooth

systemctl enable btcontrol

