// #include "../jodersky_k8055/src/k8055.h"
#ifndef USBPLATINE_H
#define USBPLATINE_H

class USBPlatine {
public:
	USBPlatine(bool debug) {};
	virtual ~USBPlatine() {};
	/// @param f_speed 0...255
	virtual void setPWM(int f_speed)=0;
	virtual void setDir(unsigned char dir)=0;
	virtual void commit()=0;
	virtual void fullstop()=0;
	/* PWM1 PWM2 digital out
	virtual int write_output ( unsigned char a1, unsigned char a2, unsigned char d )=0;
	virtual int read_input ( unsigned char *a1, unsigned char *a2, unsigned char *d, unsigned short *c1, unsigned short *c2 )=0;
*/

private:

	int debug ;
};
#endif // define USBPLATINE_H
