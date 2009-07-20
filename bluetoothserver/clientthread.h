#include "fbtctl_message.h"

struct lastStatus_t {
	int currspeed;
	int func;
	int dir;
};


class ClientThread {
public:
	ClientThread(int id, int so) : so(so), clientID(id) {};
	void run();
	void sendMessage(const FBTCtlMessage &msg);
	void setLokStatus(FBTCtlMessage &reply, lastStatus_t *lastStatus);
	void sendStatusReply(lastStatus_t *lastStatus);

	void sendClientUpdate();

	int so;
	int clientID;
};
