#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <string>

class TCPClient {
public:
	TCPClient(int id, int so) : clientID(id), so(so) {
		numClients++; // sollte atomic sein
	};
	virtual ~TCPClient();
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
	int so;
};

#endif
