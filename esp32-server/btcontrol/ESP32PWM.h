#include "USBPlatine.h"
#include "config.h"
#include "lokdef.h"


class ESP32PWM : public USBPlatine {
public:
	ESP32PWM();
	virtual ~ESP32PWM();
	/// @param f_speed 0...255
	virtual void setPWM(int f_speed);
	virtual void setDir(unsigned char dir);
	// brauch ma da ned:
	virtual void commit();
	virtual void fullstop(bool stopAll, bool emergencyStop);
  virtual void setFunction(int nFunc, bool *func);
  int sendPOM(int addr, int cv, int value);

  void loop();
  static void dumpConfig();
  
private:
	void init(int devnr);

	int dir;
	int pwm;
	char buffer[1024];

	int motorStart;
	int motorFullSpeed;
  unsigned int ledToggle;
  bool horn=false;
#ifdef HEADLIGHT_1_PIN
  bool headlight=false;
#endif
#ifdef PUTZLOK
  bool doPutz=false;
#endif
  int nFunc;
  bool currentFunc[MAX_NFUNC];
  bool changedDir=false;
};

#define BRAKEVCC 0
#define CW 1
#define CCW 2
#define BRAKEGND 3




