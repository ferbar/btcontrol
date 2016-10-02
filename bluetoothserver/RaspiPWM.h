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
	static const int maxPins=10;
	int pinsDir1[maxPins];
	int pinsDir2[maxPins];
	void init();
	void release();

	int dir;
	int pwm;

	int motorStart;

	FILE *fRaspiLed;
	int raspiLedToggle;
};
