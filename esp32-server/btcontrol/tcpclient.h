#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <string>
#include <WiFiClient.h>

class TCPClient {
public:
	TCPClient(int id, WiFiClient &client) : clientID(id), client(client) {
		numClients++; // sollte atomic sein
		this->client.setTimeout(10); // in sekunden
	};
	virtual ~TCPClient();
	virtual void run()=0;
	virtual void readSelect();
	virtual void prepareMessage();
	virtual void flushMessage();
	virtual std::string getRemoteAddr();

	virtual ssize_t read(void *buf, size_t count);
	virtual ssize_t write(const void *buf, size_t count);

	// ID vom client
	int clientID;
	// anzahl clients die gerade laufen
	static int numClients;
private:
	WiFiClient client;
};

#endif
