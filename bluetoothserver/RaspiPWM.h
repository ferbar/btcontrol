#include <map>
#include <string>
#include "USBPlatine.h"
#include "lokdef.h"
#include "Thread.h"

class RaspiPWM;

class RaspiPWMFuncThread : public Thread {
public:
	RaspiPWMFuncThread(RaspiPWM &raspiPWM) : raspiPWM(raspiPWM) {};
private:
	RaspiPWM &raspiPWM;
	virtual void run();
};

class RaspiPWM : public USBPlatine {
public:
	RaspiPWM(bool debug);
	virtual ~RaspiPWM();
	/// @param f_speed 0...255
	virtual void setPWM(int f_speed);
	virtual void setDir(unsigned char dir);
	virtual void setFunction(int nFunc, bool *func);
	// setzt die Pins, force zum initial initen
	virtual void commit();
	virtual void commit(bool force);
	virtual void fullstop(bool stopAll, bool emergencyStop);


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
	struct PinCtl {
		// PinCtl(std::string function) : function(function), lastState(UNDEFINED), pwm(100) {}
		std::string function;
		bool lastState;
		int pwm;
		typedef std::pair<int, PinCtl> pair;
	};
	std::multimap<int, PinCtl> pins;
	void dumpPins();
	Mutex funcMutex;
	RaspiPWMFuncThread funcThread;
};
