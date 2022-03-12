#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <string>
#include <WiFiClient.h>

class TCPClient {
public:
	TCPClient() : clientID(0), client() {};
	TCPClient(int id, WiFiClient &client) : clientID(id), client(client) {
		numClients++; // sollte atomic sein
		this->client.setTimeout(10); // in sekunden
		// ohne dem hat man zwischen 2 esp32 einen ping von 200-500ms
		this->client.setNoDelay(1);
	};
	void connect(int id, const IPAddress &host, int port);
	virtual void close() {
		this->client.stop();
	};
	virtual ~TCPClient();
	virtual void readSelect();
	virtual void prepareMessage();
	virtual void flushMessage();
	virtual std::string getRemoteAddr();

	virtual ssize_t read(void *buf, size_t count);
	virtual ssize_t write(const void *buf, size_t count);

	virtual bool isConnected() {
		return this->client.connected();
	};

	// ID vom client
	int clientID;
	// anzahl clients die gerade laufen
	static int numClients;
private:
	WiFiClient client;
};

#endif
