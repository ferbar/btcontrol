#include "USBPlatine"

class USBDigispark : USBPlatine {
public:
	USBDigispark(int devnr, bool debug);
	~USBDigispark();
	int write_output ( unsigned char a1, unsigned char a2, unsigned char d );
	int read_input ( unsigned char *a1, unsigned char *a2, unsigned char *d, unsigned short *c1, unsigned short *c2 );


private:
	int takeover_device( int interface );


	k8055_device *dev;
	unsigned char d;
};
