
// #define NODEBUG

#include "ControlClientThread.h"
#include "utils.h"
#include "message_layout.h"
#include "Thread.h"
#include "lokdef.h"

#define TAG "ControlClientThread"

#define MAX_QUEUE_LENGTH 5
#define CLIENT_TIMEOUT 10

ControlClientThread::ControlClientThread() : CommThread(0, CLIENT_TIMEOUT)
{
}

lokdef_t &ControlClientThread::getCurrLok()
{
  if(this->selectedAddrIndex < 0)
    throw std::runtime_error("ControlClientThread::getCurrLok() invalid selectedAddrIndex");
  int i=0;
  while(lokdef[i].addr) {
    if(this->selectedAddrIndex == i) {
      return lokdef[this->selectedAddrIndex];
    }
    i++;
  }
  throw std::runtime_error("ControlClientThread::getCurrLok() invalid selectedAddrIndex");
}


void ControlClientThread::sendPing()
{
	DEBUGF("ControlClientThread::sendPing()");
	FBTCtlMessage ping(messageTypeID("PING"));
	this->sendMessage(ping);
}

void ControlClientThread::sendFunc(int funcNr, bool enable)
{
  if(this->getQueueLength() > MAX_QUEUE_LENGTH) {
    ERRORF("sendDir: queue full!");
    return;
  }
  DEBUGF("ControlClientThread::sendFunc funcNr:%d, enable:%d", funcNr, enable);
  FBTCtlMessage cmd(messageTypeID("SETFUNC"));
  cmd["addr"]=lokdef[this->selectedAddrIndex].addr;
  cmd["funcnr"]=funcNr;
  cmd["value"]=enable;
  this->query(cmd,[](FBTCtlMessage &reply) {} );
}

void ControlClientThread::sendStop()
{
  FBTCtlMessage cmd(messageTypeID("STOP"));
  cmd["addr"]=this->getCurrLok().addr;
  this->query(cmd,[](FBTCtlMessage &reply) {} );
}

void ControlClientThread::sendDir(bool forward)
{
  if(this->getQueueLength() > MAX_QUEUE_LENGTH) {
    ERRORF("sendDir: queue full!");
    return;
  }
  FBTCtlMessage cmd(messageTypeID("DIR"));
  cmd["addr"]=this->getCurrLok().addr;
  cmd["dir"]= forward ? 1 : -1;
  this->query(cmd,[](FBTCtlMessage &reply) {} );
}

void ControlClientThread::run()
{
	DEBUGF("ControlClientThread::run()");

	try {
		auto heloMsg=this->readMessage();
		if(!heloMsg.isType("HELO")) {
			throw std::runtime_error("didn't receive HELO");
		}
		DEBUGF("server: %s, version: %s, protoHash: %#0x", 
			heloMsg["name"].getStringVal().c_str(),
			heloMsg["version"].getStringVal().c_str(),
			heloMsg["protohash"].getIntVal());
		int protocolHash=heloMsg["protohash"].getIntVal();
		if(messageLayouts.protocolHash != protocolHash) {
			throw std::runtime_error("invalid protocolHash");
		}

		while(true) {
			PRINT_FREE_HEAP("ControlClientThread::run() loop");

			CallbackCmdFunction callback=NULL;
			// wait until item in queue or 2s
			this->waitForItemInQueueTimeout();
			long timeStart=millis();
			if(this->getQueueLength() == 0) {
				this->sendPing();
			} else {
#warning todo: lock
				ControlClientThreadQueueElement item=this->cmdQueue.front();
				this->cmdQueue.pop();
				DEBUGF("/%d: sending command %s with callback (queue length: %d)", this->msgNum, messageTypeName(item.cmd.getType()).c_str(), this->getQueueLength());
				callback=item.callback;
				this->sendMessage(item.cmd);
			}
			FBTCtlMessage reply=this->readMessage();
			long timeRxReceived=millis();
			long timetaken=timeRxReceived-timeStart;
			if(this->pingMax < timetaken) this->pingMax=timetaken;
			if(this->pingMin > timetaken) this->pingMin=timetaken;
			this->pingAvg+=timetaken;
			this->pingCount++;

			DEBUGF("/%d: received message %s in %ldms", this->msgNum, messageTypeName(reply.getType()).c_str(), timetaken);
			// reply.dump();
			if(callback) {
				DEBUGF("/%d: calling query callback for message type %s", this->msgNum, messageTypeName(reply.getType()).c_str());
				callback(reply);
			}
			if(reply.isType("STATUS_REPLY")) {
				int an=reply["info"].getArraySize();
				for(int i=0; i < an; i++) {
					int addr=reply["info"][i]["addr"].getIntVal();
					int speed=reply["info"][i]["speed"].getIntVal();
					int functions=reply["info"][i]["functions"].getIntVal();
					// AvailLocosListItem item=(AvailLocosListItem)ControlAction.availLocos.get(Integer.valueOf(addr));
					DEBUGF("/%d: changed addr %i, speed: %d, functions: %0x  lokdef:%p##################", this->msgNum, addr, speed, functions, lokdef);
					if(lokdef) { // ist beim start NULL, wird erst mit GETLOCOS initialisiert
						int addr_index=getAddrIndex(addr);
						if(addr_index >=0) {
							// DEBUGF("           index=%d", addr_index);
							lokdef[addr_index].currspeed=abs(speed);
							lokdef[addr_index].currdir= (speed >= 0) ? 1 : -1;
							for(int i=0; i < MAX_NFUNC; i++) {
								// DEBUGF("       [%d] = %d", i, ( (1 << i) & functions )  ? true : false);
								lokdef[addr_index].func[i].ison=( (1 << i) & functions )  ? true : false;
							}
						// changedAddrIndex[addr_index]=true;
						}
					
					}
				}
			}
			this->msgNum++;
			this->testcancel();
		}
	} catch(const std::exception &e) { // testcancel + runtime_error
		ERRORF("ControlClientThread::run exception [%s]. closing clientthread", e.what());
		this->close();
		this->lastError=e.what();
		// rethrow
		throw;
	} catch(...) { // testcancel + runtime_error
		ERRORF("ControlClientThread::run exception. closing clientthread");
		this->close();
		// rethrow
		throw;
	}
}

void ControlClientThread::query(FBTCtlMessage cmd, CallbackCmdFunction callback)
{
	this->cmdQueue.push(ControlClientThreadQueueElement(cmd, callback));
	this->condition.signal();
}

int ControlClientThread::getQueueLength()
{
	return this->cmdQueue.size();
}

bool ControlClientThread::waitForItemInQueueTimeout()
{
	//DEBUGF("ControlClientThread::waitForItemInQueueTimeout()");
	if(this->getQueueLength() > 0)
		return true;
	return this->condition.timeoutWait(2);
}

/*
	get ping stats
private:
	cmd queue;
	int pingMax=0;
	int pingMin=0;
	int pingAvg=0;
	int pingCount=0;
*/
