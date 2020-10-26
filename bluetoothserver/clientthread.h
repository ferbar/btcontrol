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
#ifdef ESP_PLATFORM
	ClientThread(int id, WiFiClient &client) : CommThread(id, client) {
	};
#else
	ClientThread(int id, int so) : CommThread(id, so) {
	};
#endif
	
	virtual ~ClientThread();
	virtual void run();
	void setLokStatus(FBTCtlMessage &reply, lastStatus_t *lastStatus);
	void sendStatusReply(lastStatus_t *lastStatus);

	void sendClientUpdate();
};

#endif
