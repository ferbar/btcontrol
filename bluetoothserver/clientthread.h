#ifndef CLIENTTHREAD_H
#define CLIENTTHREAD_H

#include "fbtctl_message.h"

struct lastStatus_t {
	int currspeed;
	int func;
	int dir;
};


class ClientThread {
public:
	ClientThread(int id, int so) : so(so), clientID(id), msgNum(0) {
		numClients++; // sollte atomic sein
	};
	virtual ~ClientThread();
	virtual void run();
	void readSelect();
	void sendMessage(const FBTCtlMessage &msg);
	void setLokStatus(FBTCtlMessage &reply, lastStatus_t *lastStatus);
	void sendStatusReply(lastStatus_t *lastStatus);

	void sendLoco(int addr_index, bool emergencyStop);

	void sendClientUpdate();

	int so;
	// ID vom client
	int clientID;
	int msgNum;
	// anzahl clients die gerade laufen
	static int numClients;
};

#endif
