#include <memory>

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
	~SRCP();
	void pwrOn();
	void pwrOff();
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

extern SRCP *srcp;
extern const char *cfg_hostname;
extern int cfg_port;
