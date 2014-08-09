#include "USBPlatine.h"

class USBDigispark : public USBPlatine {
public:
	USBDigispark(int devnr, bool debug);
	virtual ~USBDigispark();
	virtual void setPWM(unsigned char pwm);
	virtual void setDir(unsigned char dir);
	// brauch ma da ned:
	virtual void commit() {};
	virtual void fullstop();
	// int write_output ( unsigned char a1, unsigned char a2, unsigned char d );
	// int read_input ( unsigned char *a1, unsigned char *a2, unsigned char *d, unsigned short *c1, unsigned short *c2 );

	virtual const char *readLine();

private:
	void init(int devnr);
	void release();
	int takeover_device( int interface );

	struct libusb_device_handle *devHandle;

	int dir;
	char buffer[1024];
};
