/**
 * Bluetooth Classic Example
 * Scan for devices
 * query devices for SPP - SDP profile
 * connect to first device offering a SPP connection
 * 
 * Example python server:
 * source: https://gist.github.com/ukBaz/217875c83c2535d22a16ba38fc8f2a91
 *
 * Tested with Raspberry Pi onboard Wifi/BT, USB BT 4.0 dongles, USB BT 1.1 dongles, does NOT work with USB BT 2.0 dongles when esp32 aduino lib is compiled with SSP support!
 * 
 */

#include <map>
#include "BTClient.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#ifdef CONFIG_BT_SPP_ENABLED
#warning does not work with Bluetooth 2.0 dongles, see https://github.com/espressif/esp-idf/issues/8394
#endif

#ifndef CONFIG_BT_CLASSIC_ENABLED
#error we need this!
#endif

// BluetoothSerial SerialBT;
// extBTClient btClient;


#define BT_DISCOVER_TIME  10000

/*
static bool btScanAsync = false;
static bool btScanSync = true;
*/
// extern esp_bd_addr_t _peer_bd_addr;

/*
void btAdvertisedDeviceFound(BTAdvertisedDevice* pDevice) {
}
*/

int counterfail=0;
int counterok=0;

// std::map<BTAddress, BTAdvertisedDeviceSet> btDeviceList;

void setup() {
  Serial.begin(115200);
  // btClient.enableSSP();
  if(! btClient.begin("ESP32test", true) ) {
    Serial.println("========== serialBT failed!");
    abort();
  }
  // make this device not discoverable
  esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
/*
  BTAddress addr("00:0a:cd:3b:e8:c9");
  int channel=30;
  Serial.printf("connecting to %s c%d\n", addr.toString().c_str(), channel);
  btClient.BluetoothSerial::connect(addr, channel, ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE);
return;
*/

  // esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_NONE);
  // btClient.setPin("1234");
  //btClient.enableSSP();
  // Serial.println("The device started, now you can pair it with bluetooth!");


    Serial.println("Starting discoverAsync...");
    BTScanResults* btDeviceList = btClient.getScanResults();  // maybe accessing from different threads!
    if (btClient.discoverAsync([](BTAdvertisedDevice* pDevice) {
        // BTAdvertisedDeviceSet*set = reinterpret_cast<BTAdvertisedDeviceSet*>(pDevice);
        // btDeviceList[pDevice->getAddress()] = * set;
        Serial.printf(">>>>>>>>>>>Found a new device asynchronously: %s\n", pDevice->toString().c_str());
      } )
      ) {
      delay(BT_DISCOVER_TIME);
      Serial.print("Stopping discoverAsync... ");
      btClient.discoverAsyncStop();
      Serial.println("stopped");
      delay(5000);
      if(btDeviceList->getCount() > 0) {
        BTAddress addr;
        int channel=0;
        Serial.println("Found devices:");
        for (int i=0; i < btDeviceList->getCount(); i++) {
          BTAdvertisedDevice *device=btDeviceList->getDevice(i);
          Serial.printf(" ----- %s  %s %d\n", device->getAddress().toString().c_str(), device->getName().c_str(), device->getRSSI());
          std::map<int,std::string> channels=btClient.getChannels(device->getAddress());
          Serial.printf("scanned for services, found %d\n", channels.size());
          for(auto const &entry : channels) {
            Serial.printf("     channel %d (%s)\n", entry.first, entry.second.c_str());
          }
          if(channels.size() > 0) {
            addr = device->getAddress();
            channel=channels.begin()->first;
          }
        }
        if(addr) {
          Serial.printf("connecting to %s c%d\n", addr.toString().c_str(), channel);
          bool rc=btClient.BluetoothSerial::connect(addr, channel, ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE);
          if(rc) {
            counterok++;
          } else {
            counterfail++;
          }
          // btClient.connect(addr, channel, ESP_SPP_SEC_ENCRYPT|ESP_SPP_SEC_AUTHENTICATE, ESP_SPP_ROLE_SLAVE);
          // btClient.BluetoothSerial::connect(addr, channel, ESP_SPP_SEC_ENCRYPT, ESP_SPP_ROLE_SLAVE);
        }
      } else {
        Serial.println("Didn't find any devices");
      }
    } else {
      Serial.println("Error on discoverAsync f.e. not workin after a \"connect\"");
    }

  /*
  if (btScanSync) {
    Serial.println("Starting discover...");
    BTScanResults *pResults = SerialBT.discover(BT_DISCOVER_TIME);
    if (pResults) {
      pResults->dump(&Serial);
      Serial.println("===============================================");
      for(int i=0; i < pResults->getCount(); i++) {
        BTAdvertisedDevice* dev = pResults->getDevice(i);
        if(dev) {
          Serial.printf(" %d: name %s\n", i, dev->toString().c_str());
          esp_bd_addr_t *remote_bda=dev->getAddress().getNative();
          if(false) { // ! dev->haveName()) {
            Serial.println("    sleep + query for name");
            delay(1000);
            if(esp_bt_gap_read_remote_name(*remote_bda) != ESP_OK) {
              Serial.printf("query failed");
            } else {
              Serial.println(" .... ok");
            }
          }
          if(true) {
            delay(1000);
            memcpy(_peer_bd_addr, remote_bda, ESP_BD_ADDR_LEN);
            Serial.printf("    query service %02x %02x %02x %02x %02x %02x\n", 
                _peer_bd_addr[0], _peer_bd_addr[1], _peer_bd_addr[2], _peer_bd_addr[3], _peer_bd_addr[4], _peer_bd_addr[5]);
            //if(esp_bt_gap_get_remote_services(*remote_bda) != ESP_OK) {
            /*
            esp_bt_uuid_t uuid;
            uuid.len=ESP_UUID_LEN_16;
            uuid.uuid.uuid16=0x1101;
            if(esp_bt_gap_get_remote_service_record(*remote_bda, &uuid) != ESP_OK) {
              
            * /
            /*
            if(esp_spp_start_discovery(*remote_bda) != ESP_OK) {
              Serial.printf("service query failed");
            } else {
              Serial.println(" .... ok");
            }
            * /
            SerialBT.connect(*remote_bda);
            break;
          }
          delay(5000);
          Serial.println("    sleep done");
        } else {
          Serial.println("dev null");
        }
      }
    } else
      Serial.println("Error on BT Scan, no result!");
  }
  // https://github.com/espressif/esp-idf/blob/c04803e88b871a4044da152dfb3699cf47354d18/examples/bluetooth/bluedroid/classic_bt/bt_discovery/main/main.c
  //esp_bt_gap_get_remote_services(p_dev->bda);
  Serial.println("setup() done");
  */
}


String sendData="Hi from esp32!\n";

void loop() {
  if(! btClient.isClosed() && btClient.isConnected()) {
    if( btClient.write((const uint8_t*) sendData.c_str(),sendData.length()) != sendData.length()) {
      Serial.println("tx: error");
    } else {
      Serial.printf("tx: %s",sendData.c_str());
    }
    if(btClient.available()) {
      Serial.print("rx: ");
      while(btClient.available()) {
        int c=btClient.read();
        if(c >= 0) {
          Serial.print((char) c);
        }
      }
      Serial.println();
    }
  } else {
    Serial.println("not connected");
    BTAddress addr("00:0a:cd:3b:e8:c9");
    int channel=30;
    Serial.printf("connecting to %s c%d\n", addr.toString().c_str(), channel);
    bool rc=btClient.BluetoothSerial::connect(addr, channel, ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE);
    if(rc) {
            counterok++;
    } else {
            counterfail++;
    }
    Serial.printf("ok:%d fail:%d\n", counterok, counterfail);
  }
  delay(1000);
}
