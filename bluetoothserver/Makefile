
all:
	g++ -g -lbluetooth -lpthread "../usb k8055/k8055.cpp" srcp.cpp -lusb main.cpp -o btserver -DSVNVERSION=`svnversion`

copy:
	scp /home/chris/dev/svn_btcontroll/ussp-push-0.11/src/ussp-push \
		/home/chris/dev/svn_btcontroll/MidpBluetoothExample2/MobileApplication/dist/btcontroll.jar \
		btserver \
		/home/chris/download/lgb/DDL-xmas2005/erddcd/erddcd \
		root@ibm-desktop:/root/bin/
