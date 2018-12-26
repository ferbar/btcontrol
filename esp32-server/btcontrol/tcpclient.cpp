#include "tcpclient.h"
#include "utils.h"

int TCPClient::numClients=0;

TCPClient::~TCPClient() {
}

void TCPClient::readSelect() {
}

void TCPClient::prepareMessage() {
}

void TCPClient::flushMessage() {
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
