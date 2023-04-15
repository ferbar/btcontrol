#include <string>

#define NODEBUG

#include "BTClient.h"
#include "utils.h"
#include "config.h"
// für RSSI
#include <esp_gap_bt_api.h>


#define TAG "BTClient"

#ifdef HAVE_BLUETOOTH



BTClient btClient;

void gap_callback (esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
  if (event == ESP_BT_GAP_READ_RSSI_DELTA_EVT) {
    DEBUGF("gap_callback ESP_BT_GAP_READ_RSSI_DELTA_EVT = %d", param->read_rssi_delta.rssi_delta);
    btClient.lastRSSI=param->read_rssi_delta.rssi_delta;
  }
}

bool BTClient::begin(const char* devicename) {
  DEBUGF("BTClient::begin %s", devicename);
  bool ret = BluetoothSerial::begin(devicename, true, true);
  this->setDiscoverable(false);
// ----------- clean -----------  esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
  return ret;
}

void BTClient::end() {
  DEBUGF("BTClient::end");
  PRINT_FREE_HEAP("before BT disable");
  BluetoothSerial::end();
  PRINT_FREE_HEAP("after BT disable");
  // free up ~100kb bluetooth - ram, this can only be reverted with a reset
  BluetoothSerial::memrelease();
  NOTICEF("disabling BT to get some ram... BT will be unavailable til next restart");
  PRINT_FREE_HEAP("after BT disable");
}

/**
 *
 */
void BTClient::connect(const BTAddress &address, int channel) {
	DEBUGF("connecting to %s c%d", address.toString().c_str(), channel);
	if(! BluetoothSerial::connect(address, channel, ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE)) {
		throw std::runtime_error("error connecting...");	
	} else {
		this->remoteAddress=address;
	}
	DEBUGF("connect done");
	esp_bt_gap_register_callback(gap_callback);
}

void BTClient::close() {
	DEBUGF("BTClient::disconnect()");
	BluetoothSerial::disconnect();
}


BTClient::~BTClient() {
	BluetoothSerial::disconnect();
}

void BTClient::readSelect(int timeout) {
	DEBUGF("BTClient::readSelect()");
	BluetoothSerial::setTimeout(timeout * 1000); // s -> ms
	if(BluetoothSerial::peek() < 0) {
		throw std::runtime_error("timeout/error in readSelect");
	}
}

void BTClient::prepareMessage() {
}

void BTClient::flushMessage() {
}

std::string BTClient::getRemoteAddr() {
	return this->remoteAddress.toString().c_str();
}

ssize_t BTClient::read(void *buf, size_t count) {
	DEBUGF("BTClient::read %d bytes", count);
	ssize_t readPos=0;
	long startTime=millis();
	unsigned char *data=(unsigned char*) buf;
	while(readPos < count) {
		if(millis() > startTime+2000) {
			throw std::runtime_error("::read timeout");
		}
		BluetoothSerial::setTimeout(2000);
		int c=BluetoothSerial::read();
		if(c >= 0) {
			data[readPos]=c;
			readPos++;
		} else {
			throw std::runtime_error(utils::format("::read failed after %d bytes", readPos));
		}
	}
	DEBUGF("read done");
	return readPos;
}

ssize_t BTClient::write(const void *buf, size_t count) {
	DEBUGF("BTClient::write %u bytes", count);
	ssize_t ret = BluetoothSerial::write((const uint8_t*) buf, count);
	static long lastRSSIquery=0;
	// alle 2s rssi refreshen:
	if(lastRSSIquery < millis() - 2000) {
		esp_bt_gap_read_rssi_delta(*(this->remoteAddress.getNative()));
		lastRSSIquery=millis();
	}
	return ret;
}

bool BTClient::isConnected() {
	// schaltet man die remote seite aus bleibt das noch '1'
	bool ret=BluetoothSerial::connected(0);
    DEBUGF("BTClient::isConnected() = %d", ret);
    return ret;
};

#endif
