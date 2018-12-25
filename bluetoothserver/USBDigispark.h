#include "USBPlatine.h"

class USBDigispark : public USBPlatine {
public:
	USBDigispark(int devnr, bool debug);
	virtual ~USBDigispark();
	/// @param f_speed 0...255
	virtual void setPWM(int f_speed);
	virtual void setDir(unsigned char dir);
	// brauch ma da ned:
	virtual void commit() {};
	virtual void fullstop(bool stopAll, bool emergencyStop);
	// int write_output ( unsigned char a1, unsigned char a2, unsigned char d );
	// int read_input ( unsigned char *a1, unsigned char *a2, unsigned char *d, unsigned short *c1, unsigned short *c2 );

	virtual const char *readLine();

	struct libusb_device_handle *devHandle;

private:
	void init(int devnr);
	void release();
	int takeover_device( int interface );


	int dir;
	int pwm;
	char buffer[1024];

	int motorStart;
};
