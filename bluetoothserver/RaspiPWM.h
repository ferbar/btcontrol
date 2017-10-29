#include <map>
#include <string>
#include "USBPlatine.h"
#include "lokdef.h"

class RaspiPWM : public USBPlatine {
public:
	RaspiPWM(bool debug);
	virtual ~RaspiPWM();
	/// @param f_speed 0...255
	virtual void setPWM(int f_speed);
	virtual void setDir(unsigned char dir);
	virtual void setFunction(int nFunc, bool *func);
	// brauch ma da ned:
	virtual void commit() ;
	virtual void fullstop();


private:
	static const int maxPins=10;
	void init();
	void release();

	int dir;
	int pwm;

	int motorStart;
	int motorFullSpeed;
	int motorFullSpeedBoost;

	FILE *fRaspiLed;
	int raspiLedToggle;
	bool currentFunc[MAX_NFUNC];
	int nFunc;
	std::map<int, std::string> pins;
};