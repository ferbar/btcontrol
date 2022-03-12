#pragma once

#include <map>
#include <string>
#include "Thread.h"
#include <Arduino.h>
#include "config.h"
#ifdef HAVE_BLUETOOTH
#include <BTAddress.h>
#endif

class RefreshWifiThread : public Thread {
public:
  RefreshWifiThread() {};
  void run();
  enum wifi_bt {WIFI, BLUETOOTH, UNDEFINED};
  struct Entry {int rssi=0; bool have_LR=0; wifi_bt type;
#ifdef HAVE_BLUETOOTH
  BTAddress addr;
  int channel=0;
#endif
    Entry(int rssi, bool have_LR) : rssi(rssi), have_LR(have_LR), type(WIFI) {};
#ifdef HAVE_BLUETOOTH
    Entry(const BTAddress &addr, int channel, int rssi) : rssi(rssi), type(BLUETOOTH), addr(addr), channel(channel) {};
#endif
    Entry() : type(UNDEFINED) {};
    void dump() const { Serial.printf("Entry type:%s rssi:%d"
#ifdef HAVE_BLUETOOTH
    " channel:%d"
#endif
    "\n", type==WIFI ? "WIFI" : "BLUETOOTH", rssi
#ifdef HAVE_BLUETOOTH
    , channel
#endif
    ); };
  };
  static std::map <String, Entry > wifiList;
  Mutex listMutex;
  bool listChanged=false;

  virtual const char *which() { return "RefreshWifiThread"; };
};
