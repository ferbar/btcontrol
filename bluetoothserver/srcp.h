#ifndef SRCP_H
#define SRCP_H

#include <memory>
#include "Hardware.h"

class SRCPReply {
public:
	SRCPReply(const char *message);
	~SRCPReply();

	enum SRCPReplyCodeType {INFO, OK, ERROR};

	SRCPReplyCodeType type;
	double timestamp;
	char *message;
	int code;
	void operator = (const SRCPReply &rhs); // gibts nicht
};

typedef std::auto_ptr<SRCPReply> SRCPReplyPtr;


class SRCP {
public:
	SRCP();
	virtual ~SRCP();
	virtual void pwrOn();
	virtual void pwrOff();
	static bool powered;

	SRCPReplyPtr sendLocoInit(int addr, int nFahrstufen, int nFunc);
	// add:0... dir:0 zurück 1 vor 2 notstop, nFahrstufen: 14, ,nFunc:4 
	SRCPReplyPtr sendLocoSpeed(int addr, int dir, int nFahrstufen, int speed, int nFunc, bool *func);
	SRCPReplyPtr sendPOM(int addr, int cv, int value);
	SRCPReplyPtr sendPOMBit(int addr, int cv, int bitNr, bool value);
	bool getInfo(int addr, int *dir, int *dccSpeed, int nFunc, bool *func);

	SRCPReplyPtr sendMessage(const char *msg);
	SRCPReplyPtr readReply();


private:
	int so;
};

// verwendet lokdef.h etc
class SRCP_Hardware : public Hardware, public SRCP {
	virtual void pwrOn()  { SRCP::pwrOn(); };
	virtual void pwrOff() { SRCP::pwrOff(); };
	virtual bool getPowerState() { return this->powered; }; 

	void fullstop(bool stopAll, bool emergenyStop);
	void sendLoco(int addr_index, bool emergencyStop);
	virtual int sendPOM(int addr, int cv, int value) { return SRCP::sendPOM(addr, cv, value)->type != SRCPReply::OK; };
	virtual int sendPOMBit(int addr, int cv, int bitNr, bool value) { return SRCP::sendPOMBit(addr, cv, bitNr, value)->type != SRCPReply::OK; };
};

extern const char *cfg_hostname;
extern int cfg_port;

#endif
