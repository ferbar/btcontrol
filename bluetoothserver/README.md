### Bluetooth Server
🚂🚃🚃🚃

- raspberry pi mit hardware
- Linux PC mit SRCPD + Booster Board


## Setup mit dietpi (2021.02)

dietpi-config:
Audio Options -> install alsa
bluetooth on
swap space weg
Network Options Adapters: IPv6 off
    Wifi: Auto reconnect on
Network Options Misc: Boot Net Wait: off


apt-get install git build-essential pkg-config wiringpi libbluetooth-dev libasound2-dev \
libboost-all-dev avahi-daemon
===
apt-get install vim less openssh-client lsof

mkdir -p /root/dev/
git clone git@github.com:ferbar/btcontrol  oder mit https://

optional:
git config --global --replace-all core.pager ""

make
conf Verzeichnis anpassen
make install

./btserver

app sollte jetzt die Lok finden


dietpi-services:
disable cron (haut ned hin)
systemctl disable cron
rm -rf /var/lib/dhcp
ln -s /run/ /var/lib/dhcp
ln -s /run /var/lib/run
rm /etc/resolv.conf && ln -s /run/resolv.conf /etc/
mv /var/tmp/ /var/tmp-org/ ln -s /tmp /var/run

Anpassungen /lib/systemd/system/systemd-timesyncd.service
#PrivateTmp=yes
StateDirectory=run/timesync

dietpi-drive_manager:
root und /boot readonly mounten
/ muss in der fstab auf ro gestellt werden


/boot/dietpi.txt
CONFIG_CHECK_DIETPI_UPDATES=0
CONFIG_CHECK_APT_UPDATES=0

vi /etc/vim/vimrc.local
" Explicitly source defaults.vim so you can override its settings
source $VIMRUNTIME/defaults.vim
" Prevent it from being loaded again later if the user doesn't have
" a vimrc
let skip_defaults_vim = 1
" Disable the settings you don't like
set mouse=

fscheck on boot raus aus der config / cmdline.txt

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! sound config !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
=> usb soundkarte wird standardmässig nicht als default genommen
Fehlermeldung:
aplay: set_params:1339: Sample format non available

aplay --dump-hw-params <wavfile>
cat /etc/asound.conf 
defaults.pcm.card 1
defaults.ctl.card 1



hdmi output abdrehen => hdmi-off.service

