#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <string>

class TCPClient {
public:
	TCPClient(int so) : so(so) {
	};
	virtual ~TCPClient();
	virtual void close();
	virtual void readSelect();
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
