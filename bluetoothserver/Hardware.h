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

	virtual void clientConnected() {};
	virtual void clientDisconnected(bool all) {};

	static const int CV_CV_SOUND_VOL=-10;
	static const int CV_CV_BAT=-20;
	static const int CV_CV_WIFI_CLIENT_SWITCH=-30;
	static const int CV_SOUND_VOL=266;
	static const int CV_BAT=500;
	static const int CV_WIFI_CLIENT_SWITCH=510;
	// @return: -1 => Fehler, 0..255 current CV Value
	// CV -1  => get hardware ID
	// CV -10 => get soundvol CV register
	// CV -20 => get bat voltage CV register
	virtual int sendPOM(int addr, int cv, int value)=0;
	virtual int sendPOMBit(int addr, int cv, int bitNr, bool value)=0;
};

extern Hardware *hardware;
#endif
