#ifndef CLIENTSTREAM_H
#define CLIENTSTREAM_H

#include <string>

class ClientStream {
public:
	virtual ~ClientStream() {};
	virtual void close()=0;
	virtual void readSelect(int timeout)=0;
	virtual void prepareMessage()=0;
	virtual void flushMessage()=0;
	virtual std::string getRemoteAddr()=0;

	virtual ssize_t read(void *buf, size_t count)=0;
	virtual ssize_t write(const void *buf, size_t count)=0;

	virtual bool isConnected()=0;

private:
};

#endif
