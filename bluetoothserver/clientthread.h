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
	ClientThread(int id, int so) : TCPClient(so, id), msgNum(0) {
	};
	virtual ~ClientThread();
	virtual void run();
	void sendMessage(const FBTCtlMessage &msg);
	void setLokStatus(FBTCtlMessage &reply, lastStatus_t *lastStatus);
	void sendStatusReply(lastStatus_t *lastStatus);

	void sendLoco(int addr_index, bool emergencyStop);

	void sendClientUpdate();

	int msgNum;
};

#endif
