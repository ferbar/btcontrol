
all:
	g++ -g -lbluetooth -lpthread "../usb k8055/k8055.cpp" srcp.cpp -lusb main.cpp -o btserver -DSVNVERSION=`svnversion`

copy:
	scp /home/chris/dev/svn_btcontroll/ussp-push-0.11/src/ussp-push \
		/home/chris/dev/svn_btcontroll/MidpBluetoothExample2/MobileApplication/dist/btcontroll.jar \
		btserver \
		/home/chris/download/lgb/DDL-xmas2005/erddcd/erddcd \
		root@ibm-desktop:/root/bin/
search-ibm-desktop:
	for i in `seq 1 240` ; do ping -W 1 -c 1 192.168.0.$i 2>&1| grep -B 1 "transmitted, 1"; done
