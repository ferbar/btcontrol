#ifndef CLIENTTHREAD_X11_H
#define CLIENTTHREAD_X11_H
#include "clientthread.h"

class ClientThreadX11 : public ClientThread {
public:
	ClientThreadX11(int id, int so) : ClientThread(id,so) {
	};
	~ClientThreadX11();
	virtual void run();
	/*
	void sendMessage(const FBTCtlMessage &msg);
	void setLokStatus(FBTCtlMessage &reply, lastStatus_t *lastStatus);
	void sendStatusReply(lastStatus_t *lastStatus);
*/
};

#endif
