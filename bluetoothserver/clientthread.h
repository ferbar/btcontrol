#include "fbtctl_message.h"

struct lastStatus_t {
	int currspeed;
	int func;
	int dir;
};


class ClientThread {
public:
	ClientThread(int id, int so) : so(so), clientID(id) {
		numClients++; // sollte atomic sein
	};
	~ClientThread();
	void run();
	void sendMessage(const FBTCtlMessage &msg);
	void setLokStatus(FBTCtlMessage &reply, lastStatus_t *lastStatus);
	void sendStatusReply(lastStatus_t *lastStatus);

	void BTPush(std::string addr);
	void sendClientUpdate();
	void BTScan(FBTCtlMessage &reply);

	int so;
	int clientID;
	static int numClients;
};
