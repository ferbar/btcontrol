.SUFFIXES:
.PHONY: tags clean

CFLAGS=-g -O0

all: btserver

btserver: fbtctl_message.o message_layout.o server.o btserver.o clientthread.o lokdef.o main.o srcp.o
	g++ ${CFLAGS} $+ -lbluetooth -lpthread "../usb k8055/k8055.cpp" -lusb -o btserver -DSVNVERSION=`svnversion`

help:
	@echo -e "all\t alles builden\ntags\t c-tags generieren\ncopy\t auf ibm kiste kopieren\nsearch-ibm-desktop\t ibm kiste suchen\n"

copy:
	cp ../ussp-push-0.11/src/ussp-push \
		../MidpBluetoothExample2/MobileApplication/dist/btcontroll.jar \
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
	g++ ${CFLAGS} -c -o $@  $(@:%.o=%.cpp)

tags:
	find . | grep -e '\.h$$' -e '\.c$$' -e '\.cpp$$' | \
		ctags --language-force=c++ --c++-kinds=+p --extra=+q --fields=+iaS -L-

clean:
	rm -f *.o
