#include "../jodersky_k8055/src/k8055.h"

class USBPlatine {
public:
	USBPlatine(bool debug) {};
	virtual ~USBPlatine() {};
	// PWM1 PWM2 digital out
	virtual int write_output ( unsigned char a1, unsigned char a2, unsigned char d )=0;
	virtual int read_input ( unsigned char *a1, unsigned char *a2, unsigned char *d, unsigned short *c1, unsigned short *c2 )=0;

private:

	int debug ;
};
