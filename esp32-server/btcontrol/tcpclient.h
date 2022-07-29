#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <string>
#include <WiFiClient.h>
#include "ClientStream.h"

class TCPClient : public ClientStream {
public:
	TCPClient() : client() {};
	TCPClient(WiFiClient &client) : client(client) {
//		numClients++; // sollte atomic sein
		this->client.setTimeout(10); // in sekunden
		// ohne dem hat man zwischen 2 esp32 einen ping von 200-500ms - kann auch das esp 1.0.6 sdk!!
		this->client.setNoDelay(1);
	};
	void connect(const IPAddress &host, int port);
	virtual void close() {
		this->client.stop();
	};
	virtual ~TCPClient();
	virtual void readSelect(int timeout);
	virtual void prepareMessage();
	virtual void flushMessage();
	virtual std::string getRemoteAddr();

	virtual ssize_t read(void *buf, size_t count);
	virtual ssize_t write(const void *buf, size_t count);

	virtual bool isConnected() {
		return this->client.connected();
	};

	// anzahl clients die gerade laufen => CommThread.h
//	static int numClients;
private:
	WiFiClient client;
};

#endif
