#include "USBPlatine.h"

class RaspiPWM : public USBPlatine {
public:
	RaspiPWM(bool debug);
	virtual ~RaspiPWM();
	/// @param f_speed 0...255
	virtual void setPWM(int f_speed);
	virtual void setDir(unsigned char dir);
	// brauch ma da ned:
	virtual void commit() {};
	virtual void fullstop();


private:
	void init();
	void release();

	int dir;
	int pwm;

	int motorStart;
};
