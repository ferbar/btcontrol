#ifndef CLIENTTHREAD_H
#define CLIENTTHREAD_H

#include "fbtctl_message.h"
#include "tcpclient.h"
#include "Thread.h"

#define MAX_MESSAGE_SIZE 10000


class CommThread : public TCPClient, public Thread {
public:
#ifdef ESP_PLATFORM
	CommThread() : TCPClient(), msgNum(0) {};
	CommThread(int id, WiFiClient &client) : TCPClient(id, client), msgNum(0) {
	};
	void connect(int id, const IPAddress &host, int port ) { TCPClient::connect(id, host, port); msgNum=0; };
#else
	CommThread(int id, int so) : TCPClient(id, so), msgNum(0) {
	};
#endif
	
	virtual ~CommThread();
	virtual void run()=0;
	void sendMessage(const FBTCtlMessage &msg);
	FBTCtlMessage readMessage();

	int msgNum;
};

#endif
