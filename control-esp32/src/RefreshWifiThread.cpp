
#include <WiFi.h>
#include "RefreshWifiThread.h"
#include "utils.h"
#include "config.h"
#ifdef HAVE_BLUETOOTH
#include "BTClient.h"
#endif

#define TAG "RefreshWifiThread"

std::map <String, RefreshWifiThread::Entry> RefreshWifiThread::wifiList;

void RefreshWifiThread::run() {
  DEBUGF("[%x]::run()", this->getId());
  if( WiFi.isConnected() ) {
    WiFi.disconnect();
    delay(100);
  }

#ifdef HAVE_BLUETOOTH
#warning mem_release(BLE) checken
  PRINT_FREE_HEAP("before btclient start");
  btClient.begin(device_name, true);
  BTScanResults* btDeviceList = btClient.getScanResults();  // maybe accessing from different threads!
  PRINT_FREE_HEAP("after btclient start");
#endif
  // WiFi.mode(WIFI_OFF); => schaltet Wifi komplett ab
  try {
    while(true) {  // beendet durch pthread_testcancel()
      int16_t n = WiFi.scanNetworks(); // blocking
      DEBUGF("WiFi scan done found %d networks", n);
      std::map <String, RefreshWifiThread::Entry> newList; // getChannel() dauert ein paar sekunden
      for(int i=0; i < n; i++) {
        wifi_ap_record_t *scanResult=(wifi_ap_record_t *)WiFi.getScanInfoByIndex(i);
        NOTICEF("  - found wifi %s rssi:%d, chan:%d, %d %d %d %d", scanResult->ssid, scanResult->rssi, scanResult->primary, scanResult->phy_11b, scanResult->phy_11g, scanResult->phy_11n, scanResult->phy_lr);

        auto it = newList.find(WiFi.SSID(i));
        if(it == newList.end()) {
          newList[WiFi.SSID(i)] = Entry(scanResult->rssi, scanResult->phy_lr? true : false);
        } else {
          DEBUGF("  found duplicate SSID: %s (old:%d, new:%d)", WiFi.SSID(i).c_str(), it->second.rssi, WiFi.RSSI(i));
          // multiple APs for the same ssid found, use the lower rssi;
          if(newList[WiFi.SSID(i)].rssi < WiFi.RSSI(i)) // rssi is always negative!! -40 is better than -90!
            newList[WiFi.SSID(i)].rssi = WiFi.RSSI(i) ;
        }
      }
#ifdef HAVE_BLUETOOTH
      btClient.discoverAsyncStop();
      NOTICEF("found %d bluetooth devices", btDeviceList->getCount());
      for (int i=0; i < btDeviceList->getCount(); i++) {
        BTAdvertisedDevice *device=btDeviceList->getDevice(i);
        NOTICEF("Bluetooth device: %s  %s %d", device->getAddress().toString().c_str(), device->getName().c_str(), device->getRSSI());
        String name=String("(B) ") + device->getName().c_str();
        if(newList.find(name) == newList.end()) { // not yet in list
          std::map<int,std::string> channels=btClient.getChannels(device->getAddress());
          auto it = channels.find(30);
          if(it != channels.end()) {
            DEBUGF("add %s", name.c_str());
            newList[name]=Entry(device->getAddress(), 30, device->getRSSI());
          } else {
            newList[name]=Entry(device->getAddress(), -1, device->getRSSI());
          }

        } else {
          DEBUGF("already in list");
        }
      }
      DEBUGF("bt find done");
      NOTICEF("Starting Bluetooth discoverAsync...");
      if (btClient.discoverAsync([](BTAdvertisedDevice* pDevice) {
            // BTAdvertisedDeviceSet*set = reinterpret_cast<BTAdvertisedDeviceSet*>(pDevice);
            // btDeviceList[pDevice->getAddress()] = * set;
            NOTICEF(">>>>>>>>>>>Found a new device asynchronously: %s", pDevice->toString().c_str());
            } )
            ) {
            DEBUGF("started");
      } else {
            ERRORF("Error starting bt scan");
      }
#endif
      Lock lock(this->listMutex);
      this->wifiList=newList;
      this->listChanged=true;
      lock.unlock();
      this->testcancel();
	  for(int i=0; i < 5; i++) {
        sleep(1);
        // DEBUGF("sleep done");
        this->testcancel();
      }
    }
  } catch(...) {
#ifdef HAVE_BLUETOOTH
    DEBUGF("::run() Exception - stopping discover");
    btClient.discoverAsyncStop();
#endif
    this->wifiList.clear();
    DEBUGF("[%x]::run() Exception - stopped", this->getId());
    throw;
  }
}
