#include <queue>
#include <functional>
#include "Thread.h"
#include "CommThread.h"

// c++11 muss das schon können
typedef std::function<void(FBTCtlMessage &)> CallbackCmdFunction;


class ControlClientThreadQueueElement {
public:
	ControlClientThreadQueueElement(FBTCtlMessage &cmd, CallbackCmdFunction &callback) : cmd(cmd), callback(callback) {};
	FBTCtlMessage cmd;
	CallbackCmdFunction callback;
};


class ControlClientThread : public CommThread {
public:
	ControlClientThread();
	virtual void run();
	// send command, exec callback wenn command received
	void query(FBTCtlMessage cmd, CallbackCmdFunction callback);
	// get ping stats
	int getQueueLength();
	void sendPing();

	int pingMax=0;
	int pingMin=0;
	int pingAvg=0;
	int pingCount=0;
private:
	bool waitForItemInQueueTimeout();
	std::queue<ControlClientThreadQueueElement> cmdQueue;

	Condition condition;
};
