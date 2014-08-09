.SUFFIXES:
.PHONY: tags clean

CFLAGS=-g -O0 -Wall
CFLAGS_DEP=-MMD -MF $(dir $@).$(notdir $(basename $@)).d

LDFLAGS=-g -lpthread -lstdc++
# LDFLAGS=-g -lbluetooth -lpthread -lusb

CPP=gcc

all: btserver

# extra config:
-include Makefile.config

Makefile.config:
	@echo -e "no Makefile.config - generating new config for you - if you want a new config: \n" \
		"\trm Makefile.config; make"
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
	@bluez=$$(pkg-config --libs bluez) ;\
		if [ $$? = 0 ] ; then \
			echo "found" ;\
			echo "LDFLAGS+=$$bluez" >> Makefile.config	;\
			echo "CFLAGS+=-DINCL_BT" >> Makefile.config	;\
			echo "btserver: btserver.o;" >> Makefile.config   ;\
		else						\
			echo "not found - BT disabled"	;\
		fi
	@echo .
	

btserver: fbtctl_message.o message_layout.o server.o clientthread.o lokdef.o main.o srcp.o
	${CPP} ${LDFLAGS} $+ -o btserver

digispark_test: USBDigispark.cpp
	${CPP} ${LDFLAGS} $+ -DDIGISPARK_TEST -o digispark_test

help:
	@echo -e "all\t alles builden\ntags\t c-tags generieren\ncopy\t auf ibm kiste kopieren\nsearch-ibm-desktop\t ibm kiste suchen\n"

copy:
	cp ../ussp-push-0.11/src/ussp-push \
		../MidpBluetoothExample2/MobileApplication/dist/btcontrol.jar \
		btserver \
		protocol.dat \
		~/download/lgb/srcpd_trunk/srcpd/src/srcpd \
		~/download/lgb/srcpd_trunk/srcpd/init.d.suse \
		~/download/lgb/srcpd_trunk/clients/nmra-programmer/nmra-programmer \
		initbtrail.sh \
		~/bin/
search-ibm-desktop:
	nmap -sP 192.168.0.1-240
	# for i in `seq 1 240` ; do (ping -W 1 -c 1 192.168.0.$$i 2>&1| grep -B 1 "transmitted, 1") & pid=$!; done

%.o : %.cpp %.h
	${CPP} ${CFLAGS_DEP} ${CFLAGS} -c -o $@  $<
main.o : main.cpp main.h
	${CPP} ${CFLAGS_DEP} ${CFLAGS} -c -o $@  $< -DSVNVERSION=`svnversion`

tags:
	find . | grep -e '\.h$$' -e '\.c$$' -e '\.cpp$$' | \
		ctags --language-force=c++ --c++-kinds=+p --extra=+q --fields=+iaS -L-

clean:
	rm -f *.o .*.d Makefile.config

-include .*.d
