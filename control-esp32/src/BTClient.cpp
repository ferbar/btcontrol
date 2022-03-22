#include <string>
#include "BTClient.h"
#include "utils.h"
#include "config.h"

#define TAG "BTClient"

#ifdef HAVE_BLUETOOTH

#define NODEBUG


BTClient btClient;

/**
 *
 */
void BTClient::connect(const BTAddress &address, int channel) {
	DEBUGF("connecting to %s :%d", address.toString().c_str(), channel);
	if(! BluetoothSerial::connect(address, channel, ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE)) {
		throw std::runtime_error("error connecting...");	
	} else {
		this->remoteAddress=address;
	}
	DEBUGF("connect done");
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
	return this->remoteAddress.toString();
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
	return BluetoothSerial::write((const uint8_t*) buf, count);
}

bool BTClient::isConnected() {
	bool ret=BluetoothSerial::connected(0);
    DEBUGF("BTClient::isConnected() = %d", ret);
    return ret;
};

#endif
