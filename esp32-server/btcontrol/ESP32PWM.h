#include "USBPlatine.h"

class ESP32PWM : public USBPlatine {
public:
	ESP32PWM();
	virtual ~ESP32PWM();
	/// @param f_speed 0...255
	virtual void setPWM(int f_speed);
	virtual void setDir(unsigned char dir);
	// brauch ma da ned:
	virtual void commit() {};
	virtual void fullstop(bool stopAll, bool emergencyStop);

private:
	void init(int devnr);

	int dir;
	int pwm;
	char buffer[1024];

	int motorStart;
};

#define BRAKEVCC 0
#define CW 1
#define CCW 2
#define BRAKEGND 3
void motorGo(uint8_t motor, uint8_t direct);
