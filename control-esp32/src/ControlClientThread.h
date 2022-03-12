#include <queue>
#include <functional>
#include "Thread.h"
#include "CommThread.h"
#include "lokdef.h"

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

  void sendAcc();                    // drop command if queue full
  void sendBrake();
  
  void sendStop();
  void sendDir(bool forward);
  void sendFunc(int funcNr, bool enable);

	int pingMax=0;
	int pingMin=0;
	int pingAvg=0;
	int pingCount=0;

  int selectedAddrIndex=0;

  void setCurrLok(int index) { this->selectedAddrIndex=index; };
  lokdef_t &getCurrLok();

  virtual const char *which() { return "ControlClientThread"; };

	std::string lastError;

private:
	bool waitForItemInQueueTimeout();
	std::queue<ControlClientThreadQueueElement> cmdQueue;

	Condition condition;
};
