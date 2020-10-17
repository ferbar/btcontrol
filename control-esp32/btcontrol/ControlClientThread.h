#include <queue>
#include <functional>
#include "Thread.h"
#include "clientthread.h"

// c++11 muss das schon können
typedef std::function<void(FBTCtlMessage &)> CallbackCmdFunction;


class ControlClientThreadQueueElement {
public:
	ControlClientThreadQueueElement(FBTCtlMessage &cmd, CallbackCmdFunction &callback) : cmd(cmd), callback(callback) {};
	FBTCtlMessage cmd;
	CallbackCmdFunction callback;
};


class ControlClientThread : public Thread {
public:
	ControlClientThread();
	void connect(const IPAddress &hostname, int port);
	void disconnect();
	void run();
	// send command, exec callback wenn command received
	void query(FBTCtlMessage cmd, CallbackCmdFunction callback);
	// get ping stats
	int getQueueLength();
	void sendPing();
private:
	ClientThread *client=NULL;
	WiFiClient *wifiClient=NULL;
	bool waitForItemInQueueTimeout();
	std::queue<ControlClientThreadQueueElement> cmdQueue;
	int pingMax=0;
	int pingMin=0;
	int pingAvg=0;
	int pingCount=0;

	Condition condition;
};
