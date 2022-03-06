
#include <WiFi.h>
#include "RefreshWifiThread.h"
#include "utils.h"
#include "config.h"
#ifdef HAVE_BLUETOOTH
#include "BTClient.h"
#endif

#define TAG "RefreshWifiThread"

void RefreshWifiThread::run() {
  DEBUGF("RefreshWifiThread::run() %d", this->getId());
  if( WiFi.isConnected() ) {
    WiFi.disconnect();
    delay(100);
  }
  // WiFi.mode(WIFI_OFF); => dürfte wifi hin machen
  try {
    while(true) {  // beendet durch pthread_testcancel()
      int16_t n = WiFi.scanNetworks();
      DEBUGF("WiFi scan done found %d networks", n);
      Lock lock(this->listMutex);
      this->wifiList.clear();
      for(int i=0; i < n; i++) {
        wifi_ap_record_t *scanResult=(wifi_ap_record_t *)WiFi.getScanInfoByIndex(i);
        NOTICEF("  - found wifi %s rssi:%d, chan:%d, %d %d %d %d", scanResult->ssid, scanResult->rssi, scanResult->primary, scanResult->phy_11b, scanResult->phy_11g, scanResult->phy_11n, scanResult->phy_lr);

        auto it = this->wifiList.find(WiFi.SSID(i));
        if(it == this->wifiList.end()) {
          this->wifiList[WiFi.SSID(i)] = { scanResult->rssi, scanResult->phy_lr? true : false };
        } else {
          DEBUGF("  found duplicate SSID: %s (old:%d, new:%d)", WiFi.SSID(i).c_str(), it->second.rssi, WiFi.RSSI(i));
          // multiple APs for the same ssid found, use the lower rssi;
          if(this->wifiList[WiFi.SSID(i)].rssi < WiFi.RSSI(i)) // rssi is always negative!! -40 is better than -90!
            this->wifiList[WiFi.SSID(i)].rssi = WiFi.RSSI(i) ;
        }
      }
      this->listChanged=true;
      lock.unlock();
      this->testcancel();
      sleep(5);
    }
  } catch(...) {
    this->wifiList.clear();
    DEBUGF("RefreshWifiThread::run() - stopped %d", this->getId());
    throw;
  }
}
