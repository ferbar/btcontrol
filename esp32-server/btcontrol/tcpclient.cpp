#include "tcpclient.h"
#include "utils.h"

// https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFiClient.h

static const char *TAG="tcpclient";
int TCPClient::numClients=0;

TCPClient::~TCPClient() {
}

void TCPClient::readSelect() {
#warning FIXME
	for(int i=0; i < 100; i++) {
		if(!this->client.connected()) {
			throw std::runtime_error("client not connected");
		}
		if(this->client.available() > 0) {
			DEBUGF("TCPClient::readSelect available bytes after %dms",i*50);
			return;
		}
		delay(50);
	}
	DEBUGF("TCPClient::readselect: nothing to read within 5s");
}

void TCPClient::prepareMessage() {
}

void TCPClient::flushMessage() {
	this->client.flush();
}

std::string TCPClient::getRemoteAddr() {
	abort();
}

ssize_t TCPClient::read(void *buf, size_t count) {
	return this->client.read((uint8_t *) buf, count);
}

ssize_t TCPClient::write(const void *buf, size_t count) {
	return this->client.write((uint8_t *) buf, count);
}
