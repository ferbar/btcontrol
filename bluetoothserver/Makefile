.SUFFIXES:
.PHONY: tags clean help

SHELL:=/bin/bash
CFLAGS=-g -O0 -Wall -Wformat
CFLAGS_DEP=-MMD -MF $(dir $@).$(notdir $(basename $@)).d

# clock_gettime will -lrt
# -rdynamic für schönere backtraces
LDFLAGS=-g -lpthread -lstdc++ -lrt -rdynamic

CPP=g++

all: btserver

help:
	@echo -e "***** Bluetooth Server Help *****\n" \
		"\n" \
		"clean:\t cleanup object + dependencies + config\n" \
		"all (default) rebuild\n" \
		"tags:\t c-tags generieren\n" \
		"config:\t rebuild config, auto generated if it doesn't exist\n" \
		"loop:\t wait for changes and rebuild if a file has been changed\n" \
		"install:\t setup systemd scripts etc\n" \
		"\n"


# extra config: - Vorsicht: -include führt dazu dass bei einem Fehler nicht abgebrochen wird!
-include Makefile.config

Makefile.config:
	@echo -e "##### no Makefile.config - generating new config for you \n" \
	       " use make clean to update the config\n"
	@echo "checking gcc >=4.9 (colored error messages)"
	@if [ $$( (echo "4.9" ; gcc -dumpversion ) | sort -V | head -n1 ) == "4.9" ] ; then \
		echo "CPP:=g++ -fdiagnostics-color" >> Makefile.config ; \
	fi
	@echo -e "checking libusb-dev for velleman_usb_k8055 board / digispark:"
	@usblibs=$$(pkg-config --libs libusb-1.0) ;\
		if [ $$? = 0 ] ; then \
			echo "found" ;\
			echo "LDFLAGS+=$$usblibs -lm" >> Makefile.config	;\
			echo "CFLAGS+=-DHAVE_LIBUSB" >> Makefile.config	;\
			echo "btserver: ../jodersky_k8055/src/k8055.o K8055.o USBDigispark.o" >> Makefile.config   ;\
		else						\
			echo "not found - velleman_usb_k8055 board disabled"	;\
		fi
	@echo .
	
	@echo -e "checking X11-dev"
	@# X11 + X11 test + newline wegtun
	@x11libs=$$(pkg-config --libs x11 && pkg-config --libs xtst) ;\
		if [ $$? = 0 ] ; then \
			echo "found" ;\
			x11libs=$$(echo $$x11libs) ;\
			echo "LDFLAGS+=$$x11libs" >> Makefile.config	;\
			echo "CFLAGS+=-DINCL_X11" >> Makefile.config	;\
			echo "btserver: clientthread_X11.o" >> Makefile.config ;\
		else						\
			echo "not found - X11 disabled"	;\
		fi
	@echo .
	
	@echo -e "checking libqrencode (qrencode-devel)"
	@qrcodelibs=$$(pkg-config --libs libqrencode) ;\
		if [ $$? = 0 ] ; then \
			echo "found" ;\
			echo "LDFLAGS+=$$qrcodelibs" >> Makefile.config	;\
			echo "CFLAGS+=-DINCL_QRCODE" >> Makefile.config	;\
			echo "btserver: qrcode.o" >> Makefile.config ;\
		else						\
			echo "not found - qrcode disabled"	;\
		fi
	@echo .
	
	@echo -e "checking blueZ:"
	@bluezlibs=$$(pkg-config --libs bluez) ;\
		if [ $$? = 0 ] ; then \
			echo "found" ;\
			echo "LDFLAGS+=$$bluezlibs" >> Makefile.config	;\
			echo "CFLAGS+=-DINCL_BT" >> Makefile.config	;\
			echo "btserver: btserver.o" >> Makefile.config   ;\
		else						\
			echo "not found - BT disabled"	;\
		fi
	@echo .
	
	@# -std=c++11 für zsp.cpp,  -std=gnu++0x für gcc <= 6.3
	@echo -e "checking alsa:"
	@alsalibs=$$(pkg-config --libs alsa) ;\
		if [ $$? = 0 ] ; then \
			echo "found" ;\
			echo "LDFLAGS+=$$alsalibs" >> Makefile.config	;\
			echo "CFLAGS+=-DHAVE_ALSA -std=gnu++11" >> Makefile.config	;\
			echo "btserver: sound.o zsp.o" >> Makefile.config   ;\
		else						\
			echo "not found - alsa sound disabled"	;\
		fi
	@echo .
	
	@# -lwiringPi HAVE_RASPI_ACT_LED | HAVE_RASPI_WIRINGPI
	@echo -e "checking raspi ACT led:"
	@if [ -f "/sys/class/leds/led0/brightness" ] ; then \
		echo "found" ;\
		echo "CFLAGS+=-DHAVE_RASPI_ACT_LED" >> Makefile.config ; \
		echo -e "checking for wiringPI:" ; \
		if [ -f "/usr/local/lib/libwiringPi.so" -o -f "/usr/lib/libwiringPi.so" ] ; then \
			echo "found" ;		\
			echo "CFLAGS+=-DHAVE_RASPI_WIRINGPI" >> Makefile.config ; \
			echo "LDFLAGS+=-lwiringPi" >> Makefile.config;			\
			echo "btserver: RaspiPWM.o" >> Makefile.config;			\
		elif [ -f /usr/include/pigpio.h ] ; then \
			echo "found pigpio" ; 	\
			if lsmod | grep -q i2s ; then \
				echo "!!!!!!!!!!!!!!!!!!!!!!!!!! I2S soundcard + pigpio doesn't work !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"; \
				echo " you have to install wiringpi sry";		\
				rm Makefile.config ; 					\
				exit 1 ;						\
			fi; 								\
			echo "CFLAGS+=-DHAVE_RASPI_PIGPIO" >> Makefile.config ; 	\
			echo "LDFLAGS+=-lpigpio" >> Makefile.config ;			\
			echo "btserver: RaspiPWM.o" >> Makefile.config ;		\
		else					\
			echo "not found - RaspiPWM disabled"	;\
		fi ; 					\
	else						\
		echo "not found - raspi ACT led disabled"	;\
	fi
	@echo .
	
	@echo -e "checking libsndfile:"
	@sndfilelibs=$$(pkg-config --libs sndfile) ;\
		if [ $$? = 0 ] ; then \
			echo "found" ;\
			echo "all: zpp2zsp" >> Makefile.config   ;\
		else						\
			echo "not found - zpp2zsp disabled"	;\
		fi
	@echo .

	@# gibts unter linux immer (am esp32 nicht)
	@echo "CFLAGS+=-DHAVE_EXECINFO" >> Makefile.config

# Makefile.config brauch ma damit build abbricht wenn Makefile.config nicht angelegt werden kann
btserver: Makefile.config fbtctl_message.o CommThread.o message_layout.o server.o tcpclient.o clientthread.o lokdef.o main.o srcp.o utils.o ParseExpr.o Thread.o USBPlatine.o BTUtils.o
	${CPP} $(filter %.o, $+) ${LDFLAGS} -o btserver

digispark_test: USBDigispark.cpp utils.o
	${CPP} ${CFLAGS} ${LDFLAGS} $+ -DDIGISPARK_TEST -o digispark_test

zsptest: zsptest.cpp sound.cpp
	${CPP} ${CFLAGS} ${LDFLAGS} -lasound $+ -g 

zpp2zsp: zpp2zsp.cpp utils.o Thread.o zsp.o sound.o lokdef.o
	${CPP} ${CFLAGS} ${LDFLAGS} -lsndfile -lboost_locale $+ -g -o zpp2zsp

# !!! this is for esp32 only !!!
protocol.h: protocol.dat
	echo '// !!! AUTOGENERATED BY make protocol.h !!!' > protocol.h
	echo -n 'const char *protocol_dat="' >> protocol.h
	cat protocol.dat | python3 -c "import sys; sys.stdout.write(sys.stdin.read().replace('\n','\\\\n\"\n\"').replace('???','??\" \"'))" >> protocol.h
	echo '";' >> protocol.h


loop:
	@if ! which inotifywait ; then \
		echo "please install inotify-tools" ; \
		exit 1 ; \
	fi
	@reset
	@echo "$$BASH_VERSION" 
	@echo "starting make after file changes"
	#script -q --return -c "make" /dev/null | head -n $$(tput lines) ; 
	@while inotifywait -q *.cpp *.h ; do \
		reset; \
		make | head -n $$(tput lines) ; \
		if [ $${PIPESTATUS[0]} != 0 ] ; then echo -e "\b" ; fi; \
	done ;

install:
	# raspi setup or srcpd setup?
	if grep -q ID=raspbian /etc/os-release ; then \
		echo "Raspi install" ; \
		btout=$$(bluetoothctl --version) ; \
		regex='([0-9])\.[0-9]' ; \
		if [[ "$$btout" =~ $$regex ]] ; then \
			echo "blutoothctl version: $${BASH_REMATCH[1]}" ; \
			v="$${BASH_REMATCH[1]}" ; \
			if [ $$v -ge 5 ] ; then \
				if grep -q -e '--compat' /etc/systemd/system/dbus-org.bluez.service ; then \
					echo "bluez compat mode already enabled" ; \
				else \
					echo "!!! modification of /etc/systemd/system/dbus-org.bluez.service required !!!" ; \
					echo "change ExecStart=/usr/libexec/bluetooth/bluetoothd to ExecStart=/usr/libexec/bluetooth/bluetoothd --compat" ; \
					echo "reload systemd afterwards: systemctl daemon-reload; systemctl restart bluetooth" ; \
					exit 1 ; \
				fi ; \
			fi ; \
		else \
			echo "Error finding bluez version!" ; \
			exit 1 ; \
		fi ; \
		if [ ! -f ~/.bashrc-save-btcontrol -a -f ~/.bashrc ] ; then \
			mv ~/.bashrc ~/.bashrc-save-btcontrol ; \
			ln -s $$PWD/etc/bashrc ~/.bashrc ; \
		fi ; \
		if [ ! -f ~/.inputrc ] ; then \
			ln -s $$PWD/etc/inputrc ~/.inputrc ; \
		fi ; \
	else \
		cp ../ussp-push-0.11/src/ussp-push \
			../MidpBluetoothExample2/MobileApplication/dist/btcontrol.jar \
			btserver \
			protocol.dat \
			~/download/lgb/srcpd_trunk/srcpd/src/srcpd \
			~/download/lgb/srcpd_trunk/srcpd/init.d.suse \
			~/download/lgb/srcpd_trunk/clients/nmra-programmer/nmra-programmer \
			initbtrail.sh \
			~/bin/ ; \
	fi
	cp etc/avahi/services/btcontrol.service /etc/avahi/services/
	cp etc/systemd/system/btcontrol.service /etc/systemd/system/
	cp etc/systemd/system/btcontrol-initbt.service /etc/systemd/system/
	@echo "checking bluetooth discoverable on boot"
	@if grep '^DiscoverableTimeout' /etc/bluetooth/main.conf ; then \
		echo "already set!" ;			\
	else					\
		echo "adding DiscoverableTimeout = 0" ;	\
		sed -i 's/\[General\]/[General]\n# config by btcontrol\nDiscoverableTimeout = 0/' /etc/bluetooth/main.conf ; \
	fi

search-ibm-desktop:
	nmap -sP 192.168.0.1-240
	# for i in `seq 1 240` ; do (ping -W 1 -c 1 192.168.0.$$i 2>&1| grep -B 1 "transmitted, 1") & pid=$!; done

%.o : %.cpp %.h
	${CPP} ${CFLAGS_DEP} ${CFLAGS} -c -o $@  $<
main.o : main.cpp main.h
	${CPP} ${CFLAGS_DEP} ${CFLAGS} -c -o $@  $< -DSVNVERSION="git$$(git log --oneline | wc -l)"

config: Makefile.config
	echo "done"

tags:
	find . | grep -e '\.h$$' -e '\.c$$' -e '\.cpp$$' | \
		ctags --language-force=c++ --c++-kinds=+p --extra=+q --fields=+iaS -L-

clean:
	rm -f *.o .*.d Makefile.config

-include .*.d
