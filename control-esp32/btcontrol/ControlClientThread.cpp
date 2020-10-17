#include "ControlClientThread.h"
#include "utils.h"
#include "message_layout.h"
#include "Thread.h"
#include "lokdef.h"


#define TAG "ControlClientThread"

#warning FIXME: das hardware komplett raushaun - Clientthread aufspitten
#include "Hardware.h"
Hardware *hardware=NULL;

ControlClientThread::ControlClientThread()
{
}

void ControlClientThread::connect(const IPAddress &host, int port)
{
	assert(!this->wifiClient);
	this->wifiClient = new WiFiClient;
	if(this->wifiClient->connect(host, port)) {
		DEBUGF("ControlClientThread::connect - connected!");
		this->client = new ClientThread(0, *this->wifiClient);
		return;
	}
	throw std::runtime_error("error connecting");
}

void ControlClientThread::disconnect()
{
	DEBUGF("ControlClientThread::disconnect()");
	abort();
}

void ControlClientThread::sendPing()
{
	DEBUGF("ControlClientThread::sendPing()");
	FBTCtlMessage ping(messageTypeID("PING"));
	this->client->sendMessage(ping);
}

void ControlClientThread::run()
{
	DEBUGF("ControlClientThread::run()");
	assert(this->client != NULL);

	auto heloMsg=this->client->readMessage();
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
		CallbackCmdFunction callback=NULL;
		// FIXME !!!!!!!!!!!!!!!!!!!!!!!!! wait until item in queue or 2s
		this->waitForItemInQueueTimeout();
		if(this->getQueueLength() == 0) {
			this->sendPing();
		} else {
			DEBUGF("----- sending command with callback");
			#warning todo: lock
			ControlClientThreadQueueElement item=this->cmdQueue.front();
			this->cmdQueue.pop();
			callback=item.callback;
			this->client->sendMessage(item.cmd);
		}
		FBTCtlMessage reply=this->client->readMessage();
		DEBUGF("%d: msg", this->client->msgNum);
		reply.dump();
		if(callback) {
			DEBUGF("%d: callback", this->client->msgNum);
			callback(reply);
		}
		if(reply.isType("STATUS_REPLY")) {
			int an=reply["info"].getArraySize();
			for(int i=0; i < an; i++) {
				int addr=reply["info"][i]["addr"].getIntVal();
				int speed=reply["info"][i]["speed"].getIntVal();
				int functions=reply["info"][i]["functions"].getIntVal();
				// AvailLocosListItem item=(AvailLocosListItem)ControlAction.availLocos.get(Integer.valueOf(addr));
				DEBUGF("changed addr %i, speed: %d, functions: %d##################", addr, speed, functions);
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

	}

/*
	   -> send keys every 2s
	if discconnect quit
	*/
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
	/*
	int lock_rv = pthread_mutex_lock(&this->mutex.m);
	if(lock_rv) {
		throw std::runtime_error("error lock");
	}
	/ *
	pthread_timestruc_t to;
	to.tv_sec = time(NULL) + 5;
	to.tv_nsec = 0;
	* /
	struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 2;


    // rc = 0;
    // while (! mypredicate(&t) && rc == 0)
    int rc = pthread_cond_timedwait(&this->cond, &this->mutex.m, &ts);
	(void) pthread_mutex_unlock(&this->mutex.m);
	return rc;
	*/
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
