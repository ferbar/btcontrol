#include "Arduino.h"
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <errno.h>

#include "tcpclient.h"
#include "utils.h"

// https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFiClient.h

static const char *TAG="tcpclient";
int TCPClient::numClients=0;

TCPClient::~TCPClient() {
}

void TCPClient::connect(int id, const IPAddress &host, int port)
{
	this->client.stop();
	this->client.flush();
	if(this->client.connect(host, port)) {
		DEBUGF("ControlClientThread::connect - connected!");
	} else {
		throw std::runtime_error(strerror(errno));
	}
	this->clientID=id;
	this->client.setTimeout(10); // in sekunden
	// ohne dem hat man zwischen 2 esp32 einen ping von 200-500ms
	this->client.setNoDelay(1);
}

void TCPClient::readSelect() {
	if(!this->client.connected()) {
		throw std::runtime_error("client not connected");
	}

	// !!!! wificlient hat einen eigenen rx buffer -> is da schon was drinnen?
	if(this->client.available()) {
		return;
	}

	long start=millis();
	const int timeout=5 * 1000 ; // 5 sekunden

	int sockfd= this->client.fd();

// clear error flag:
	int sockerr;
	socklen_t len = (socklen_t)sizeof(int);
	int res = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &sockerr, &len);
	if(res != 0 || sockerr != 0) {
		ERRORF("readSelect: old getsockopt: res:%d, sockerr:%d", res, sockerr);
	}

	fd_set fdset;
	struct timeval tv;
	FD_ZERO(&fdset);
	FD_SET(sockfd, &fdset);
	tv.tv_sec = 0;
	tv.tv_usec = timeout * 1000;
	res = select(sockfd + 1, &fdset, nullptr, nullptr, timeout<0 ? nullptr : &tv);
	if (res < 0) {
		ERRORF("select on fd %d, errno: %d, \"%s\"", sockfd, errno, strerror(errno));
		// close(sockfd);
		throw std::runtime_error("select error");
	} else if (res == 0) {
		ERRORF("select returned due to timeout %d ms for fd %d", timeout, sockfd);
		// close(sockfd);
		throw std::runtime_error("timeout reading cmd");
	} else {
		// DEBUGF("select returned res=%d", res);
		int sockerr;
		socklen_t len = (socklen_t)sizeof(int);
		res = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &sockerr, &len);

		if (res < 0) {
			ERRORF("getsockopt on fd %d, errno: %d, \"%s\"", sockfd, errno, strerror(errno));
			throw std::runtime_error("getsockopt error");
		}

		if (sockerr != 0) {
			ERRORF("socket error on fd %d, errno: %d, \"%s\"", sockfd, sockerr, strerror(sockerr));
			throw std::runtime_error("socket error");
		}
	}
	DEBUGF("select done in %ldms",millis()-start);

/*
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
	*/
	DEBUGF("TCPClient::readselect: success");
}

void TCPClient::prepareMessage() {
	/*  ==== gibts beim freertos scheinbar nicht
	int flag = 1;
    setsockopt(this->client.fd(), IPPROTO_TCP, TCP_CORK, (char *)&flag, sizeof(flag) ); // prepare message, stopsel rein
	*/
}

void TCPClient::flushMessage() {
	// this->client.flush(); !!!!! vorsicht das löscht den read buffer !!!!
	/*  ==== gibts beim freertos scheinbar nicht
	int flag = 0;
    setsockopt(this->client.fd(), IPPROTO_TCP, TCP_CORK, (char *)&flag, sizeof(flag) ); // prepare message, stopsel rein
	*/
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
