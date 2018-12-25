#ifndef HARDWARE_H
#define HARDWARE_H

class Hardware {
public:
// kein konstruktor	Hardware(bool debug) {};
	Hardware() {};
	virtual ~Hardware() {};
	virtual void sendLoco(int addr_index, bool emergencyStop)=0;
	virtual void fullstop(bool stopAll, bool emergencyStop)=0;

	virtual void pwrOn()=0;
	virtual void pwrOff()=0;
	virtual bool getPowerState()=0;

	virtual int sendPOM(int addr, int cv, int value)=0;
	virtual int sendPOMBit(int addr, int cv, int bitNr, bool value)=0;
};

extern Hardware *hardware;
#endif
