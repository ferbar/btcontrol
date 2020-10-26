#ifndef CLIENTTHREAD_H
#define CLIENTTHREAD_H

#include "fbtctl_message.h"
#include "tcpclient.h"

#define MAX_MESSAGE_SIZE 10000

struct lastStatus_t {
	int currspeed;
	int func;
	int dir;
};


class ClientThread : public TCPClient {
public:
#ifdef ESP_PLATFORM
	ClientThread(int id, WiFiClient &client) : TCPClient(id, client), msgNum(0) {
	};
#else
	ClientThread(int id, int so) : TCPClient(id, so), msgNum(0) {
	};
#endif
	
	virtual ~ClientThread();
	virtual void run();
	void sendMessage(const FBTCtlMessage &msg);
	FBTCtlMessage readMessage();
	void setLokStatus(FBTCtlMessage &reply, lastStatus_t *lastStatus);
	void sendStatusReply(lastStatus_t *lastStatus);

	void sendClientUpdate();

	int msgNum;
};

#endif
