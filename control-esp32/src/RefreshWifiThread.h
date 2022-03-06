#pragma once

#include <map>
#include <string>
#include "Thread.h"
#include <Arduino.h>

class RefreshWifiThread : public Thread {
public:
  RefreshWifiThread() {};
  void run();
  struct Entry {int rssi; bool have_LR; enum type {WIFI, BLUETOOTH}; }; 
  static std::map <String, Entry > wifiList;
  Mutex listMutex;
  bool listChanged=false;
};
