.SUFFIXES:
.PHONY: tags clean

all: fbtctl_message.o message_layout.o
	g++ $+ -g -lbluetooth -lpthread "../usb k8055/k8055.cpp" srcp.cpp -lusb main.cpp -o btserver -DSVNVERSION=`svnversion`

copy:
	scp /home/chris/dev/svn_btcontroll/ussp-push-0.11/src/ussp-push \
		/home/chris/dev/svn_btcontroll/MidpBluetoothExample2/MobileApplication/dist/btcontroll.jar \
		btserver \
		/home/chris/download/lgb/DDL-xmas2005/erddcd/erddcd \
		root@ibm-desktop:/root/bin/
search-ibm-desktop:
	nmap -sP 192.168.0.1-240
	# for i in `seq 1 240` ; do (ping -W 1 -c 1 192.168.0.$$i 2>&1| grep -B 1 "transmitted, 1") & pid=$!; done

%.o : %.cpp
	g++ -O0 -g -c -o $@  $(@:%.o=%.cpp)

tags:
	find . | grep -e '\.h$$' -e '\.c$$' -e '\.cpp$$' | \
		ctags --language-force=c++ --c++-kinds=+p --extra=+q --fields=+iaS -L-

clean:
	rm -f *.o
