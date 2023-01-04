// #include "../jodersky_k8055/src/k8055.h"
#ifndef USBPLATINE_H
#define USBPLATINE_H

#include <string>
#include "Hardware.h"

class USBPlatine : public Hardware {
public:
	USBPlatine(bool debug);
	virtual ~USBPlatine() {};
	/// @param f_speed 0...255
	virtual void setPWM(int f_speed)=0;
	/// @param dir 0 || 1
	virtual void setDir(unsigned char dir)=0;
	/// @param func[MAX_NFUNC]
	virtual void setFunction(int nFunc, bool *func) {}; // optional
	virtual void commit()=0;
	virtual void fullstop(bool stopAll, bool emergencyStop)=0;
	/* PWM1 PWM2 digital out
	virtual int write_output ( unsigned char a1, unsigned char a2, unsigned char d )=0;
	virtual int read_input ( unsigned char *a1, unsigned char *a2, unsigned char *d, unsigned short *c1, unsigned short *c2 )=0;
*/

	virtual void clientConnected();
	virtual void clientDisconnected(bool all);

	virtual void sendLoco(int addr_index, bool emergencyStop);
	
	virtual int sendPOM(int addr, int cv, int value);
	virtual int sendPOMBit(int addr, int cv, int bitNr, bool value);

	virtual void pwrOn() {};
	virtual void pwrOff() { this->setDir(0); };
	virtual bool getPowerState() {return true; };

private:

	int debug ;
	std::string powerMonitorVoltageFile ;
	int powerMonitoringVoltageMin;
	int powerMonitoringVoltageMax;
};
#endif // define USBPLATINE_H
