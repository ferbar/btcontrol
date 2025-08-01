#include <WiFi.h>
#include <forward_list>
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
  PRINT_FREE_HEAP("before btclient start");
  if(!btClient.begin(device_name)) {
    ERRORF("Bluetooth init failed!");
  }
  PRINT_FREE_HEAP("after btclient start");

  // there's no lock and we are on a different thread => can't access except when discover is stopped or in the callback handler!!!
  // BTScanResults* btDeviceList = btClient.getScanResults();
  std::forward_list<BTAdvertisedDeviceSet> myDiscoveredDevices;
  struct ch30scans{
    int retries=0;
    bool found=false;
  };
  std::map <String, ch30scans > hasChannel30Cache;
  Mutex btDiscoveredDevicesMutex;
  PRINT_FREE_HEAP("after btclient start");

#endif
  // WiFi.mode(WIFI_OFF); => schaltet Wifi komplett ab
  try {
    while(true) {  // beendet durch pthread_testcancel()
      PRINT_FREE_HEAP("while loop start");
#ifdef HAVE_BLUETOOTH
      if(btClient.discoverInProgress()) {
        DEBUGF("BT discover in progress");
      } else {
        NOTICEF("Starting Bluetooth discoverAsync...");
        if(btClient.discoverAsync([&myDiscoveredDevices, &btDiscoveredDevicesMutex](BTAdvertisedDevice* pDevice) {
            NOTICEF(">>>>>>>>>>>Found a new device asynchronously: %s (len:%d)", pDevice->toString().c_str());
            Lock btLock(btDiscoveredDevicesMutex);
            BTAdvertisedDeviceSet *p=reinterpret_cast<BTAdvertisedDeviceSet *>(pDevice);
            // check if old device exists without name:
            auto it=myDiscoveredDevices.begin();
            bool found=false;
            while(it != myDiscoveredDevices.end()) {
              if(it->getAddress() == p->getAddress()) {
                if(p->getName() != "") {
                  DEBUGF("update existing device %s", it->toString().c_str());
                  *it=*p;
                }
                found=true;
              }
              ++it;
            }
            if(!found) {
              myDiscoveredDevices.push_front(*p);
            }
        } )
        ) {
          DEBUGF("started");
        } else {
          ERRORF("Error starting bt scan");
        }
      }
#endif
      PRINT_FREE_HEAP("while loop before scanNetworks");
      int16_t n = WiFi.scanNetworks(); // blocking
      DEBUGF("WiFi scan done found %d networks", n);
      std::map <String, RefreshWifiThread::Entry> newList;
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
      // Delete the scan result to free memory for code below.
      PRINT_FREE_HEAP("while loop before scanDelete");
      WiFi.scanDelete();
      PRINT_FREE_HEAP("while loop after scanDelete");
#ifdef HAVE_BLUETOOTH
      Lock btLock(btDiscoveredDevicesMutex);
      // NOTICEF("found %d bluetooth devices", myDiscoveredDevices.size());
      n=0;
      for (auto &it : myDiscoveredDevices) {
        n++;
      }
      NOTICEF("found %d bluetooth devices", n);
      for (auto &it : myDiscoveredDevices) {
        // BTAdvertisedDevice *device=const_cast<BTAdvertisedDeviceSet *>(&(it.second));
        BTAdvertisedDeviceSet &device=it;
        NOTICEF("Bluetooth device: %s  %s %d", device.getAddress().toString().c_str(), device.getName().c_str(), device.getRSSI());
        String name=String("\xA0(B) ");
        if(device.getName() == "") {
          name+=device.getAddress().toString(); 
        } else {
          name+=device.getName().c_str();
        }
        if(newList.find(name) == newList.end()) { // not yet in list
          // das liefert hin und wieder nix -> eventuell muss man das discover vorher stoppen?
          // getChannel() dauert ein paar sekunden
          bool add=false;
          if(hasChannel30Cache[device.getAddress().toString()].found) {
            DEBUGF("device %s has channel 30 (cached)", name.c_str());
            add=true;
          } else {
            DEBUGF("query channels (%d retries)",hasChannel30Cache[device.getAddress().toString()].retries);
            if(hasChannel30Cache[device.getAddress().toString()].retries < 10) {
              std::map<int,std::string> &channels=btClient.getChannels(device.getAddress());
              auto channel30 = channels.find(30);
              if(channel30 != channels.end()) {
                DEBUGF("found %s", name.c_str());
                add=true;
                hasChannel30Cache[device.getAddress().toString()].found=true;
              } else {
                hasChannel30Cache[device.getAddress().toString()].found=false;
                hasChannel30Cache[device.getAddress().toString()].retries++;
              }
              channels.clear();
            }
          }
          if(add) {
            DEBUGF("add %s", name.c_str());
            newList[name]=Entry(device.getAddress(), 30, device.getRSSI());
          } else {
            newList[name]=Entry(device.getAddress(), -1, device.getRSSI());
          }

        } else {
          DEBUGF("already in list");
        }
      }
      btLock.unlock();
      DEBUGF("bt find done");
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
    DEBUGF("::run() Exception - stopping bluetooth discover");
    btClient.discoverAsyncStop();
    btClient.discoverClear();
#endif
    this->wifiList.clear();
    myDiscoveredDevices.clear();
    DEBUGF("[%x]::run() Exception - stopped", this->getId());
    throw;
  }
}
