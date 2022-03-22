#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <string>
#include "ClientStream.h"

class TCPClient : public ClientStream {
public:
	TCPClient(int so) : so(so) {
	};
	virtual ~TCPClient();
	virtual void close();
	virtual void readSelect(int timeout);
	virtual void prepareMessage();
	virtual void flushMessage();
	virtual std::string getRemoteAddr();

	virtual ssize_t read(void *buf, size_t count);
	virtual ssize_t write(const void *buf, size_t count);

	virtual bool isConnected() { return this->so >= 0; } ;

private:
	int so;
};

#endif
