#include "ControlClientThread.h"
#include "utils.h"
#include "message_layout.h"
#include "Thread.h"
#include "lokdef.h"


#define TAG "ControlClientThread"


ControlClientThread::ControlClientThread()
{
}

#warning cleanup 
/*
void ControlClientThread::connect(const IPAddress &host, int port)
{
	
	this->wifiClient = new WiFiClient;
	if(this->wifiClient->connect(host, port)) {
		DEBUGF("ControlClientThread::connect - connected!");
		this->client = new ClientThread(0, *this->wifiClient);
		return;
	}
	delete this->wifiClient;
	this->wifiClient = NULL;
	throw std::runtime_error("error connecting");
}
void ControlClientThread::disconnect()
{
	DEBUGF("ControlClientThread::disconnect()");
	abort();
}
*/


void ControlClientThread::sendPing()
{
	DEBUGF("ControlClientThread::sendPing()");
	FBTCtlMessage ping(messageTypeID("PING"));
	this->sendMessage(ping);
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
			// DEBUGF("TCPClient::readSelect() watermark: %d", uxTaskGetStackHighWaterMark(NULL));

			CallbackCmdFunction callback=NULL;
			// wait until item in queue or 2s
			this->waitForItemInQueueTimeout();
			long timeStart=millis();
			if(this->getQueueLength() == 0) {
				this->sendPing();
			} else {
				DEBUGF("/%d: sending command with callback", this->msgNum);
				#warning todo: lock
				ControlClientThreadQueueElement item=this->cmdQueue.front();
				this->cmdQueue.pop();
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

			DEBUGF("/%d: received message in %ldms", this->msgNum, timetaken);
			reply.dump();
			if(callback) {
				DEBUGF("/%d: callback", this->msgNum);
				callback(reply);
			}
			if(reply.isType("STATUS_REPLY")) {
				int an=reply["info"].getArraySize();
				for(int i=0; i < an; i++) {
					int addr=reply["info"][i]["addr"].getIntVal();
					int speed=reply["info"][i]["speed"].getIntVal();
					int functions=reply["info"][i]["functions"].getIntVal();
					// AvailLocosListItem item=(AvailLocosListItem)ControlAction.availLocos.get(Integer.valueOf(addr));
					DEBUGF("/%d: changed addr %i, speed: %d, functions: %d##################", this->msgNum, addr, speed, functions);
					if(lokdef) { // ist beim start NULL, wird erst mit GETLOCOS initialisiert
						int addr_index=getAddrIndex(addr);
						if(addr_index >=0) {
							// DEBUGF("           index=%d:", addr_index);
							lokdef[addr_index].currspeed=abs(speed);
							lokdef[addr_index].currdir= (speed >= 0) ? 1 : -1;
							for(int i=0; i < MAX_NFUNC; i++) {
								lokdef[addr_index].func[i].ison=(1 >> i) | functions ? true : false;
							}
						// changedAddrIndex[addr_index]=true;
						}
					
					}
				}
			}
			this->msgNum++;
		}
	} catch(std::runtime_error &e) {
		ERRORF("ControlClientThread::run exception. closing clientthread");
		this->disconnect();
		// rethrow
		throw e;
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
	DEBUGF("ControlClientThread::waitForItemInQueueTimeout()");
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
