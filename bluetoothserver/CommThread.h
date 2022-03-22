#ifndef CLIENTTHREAD_H
#define CLIENTTHREAD_H

#include "fbtctl_message.h"
#include "ClientStream.h"
#include "Thread.h"

#define MAX_MESSAGE_SIZE 10000


class CommThread : public Thread {
public:
	CommThread(int clientID, int defaultTimeout) : clientID(clientID), timeout(defaultTimeout) {
		numClients++; // sollte atomic sein
	};
	void begin(ClientStream *client, bool doDelete) { this->client=client; this->doDelete = doDelete; this->msgNum=0; };
	virtual ~CommThread();
	virtual void run()=0;
	void sendMessage(const FBTCtlMessage &msg);
	FBTCtlMessage readMessage();

	void readSelect();

	void close();
    
	ClientStream *client=NULL;
	bool doDelete=false;

	int msgNum=0;

	// ID vom client
	int clientID;

	// anzahl clients die gerade laufen
	static int numClients;
private:
	int timeout=0;
};

#endif
