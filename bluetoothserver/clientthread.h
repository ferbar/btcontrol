#ifndef COMMTHREAD_H
#define COMMTHREAD_H

#include "CommThread.h"


struct lastStatus_t {
	int currspeed;
	int func;
	int dir;
};


class ClientThread : public CommThread {
public:
	ClientThread(int clientID, int defaultTimeout) : CommThread(clientID, defaultTimeout) {};
// void begin(Client)
	virtual ~ClientThread();
	virtual void run();
	void setLokStatus(FBTCtlMessage &reply, lastStatus_t *lastStatus);
	void sendStatusReply(lastStatus_t *lastStatus);

	void sendClientUpdate();
};

#endif
