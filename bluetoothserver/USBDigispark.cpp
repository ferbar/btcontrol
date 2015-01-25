/**
 * PWM über einen digispark
 * vorallem die sleeps sind aus digispark.c monitor
 * hint zum usb sniffen:
 *   mount -t debugfs none_debugs /sys/kernel/debug
 *   tail -f /sys/kernel/debug/usbmon/0u
 * libusb debuggen: libusb_set_debug()
 */
#include <stdio.h>
#include <string.h>
#include <libusb-1.0/libusb.h>        // libusb-1.0
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "USBDigispark.h"
#include "utils.h"

bool useCommthread=true;

char read(libusb_device_handle *devHandle);

static libusb_context* context = NULL;

pthread_t commThread;
int x,y;
// statt dem da semaphore mit        semop, semtimedop - System V semaphore operations ??
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
char commBuffer[1024];
pthread_mutex_t buffer_empty_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_empty_cond = PTHREAD_COND_INITIALIZER;

USBDigispark *usbDigispark;

float timediff(timespec &start, timespec &done) {
	float ret = done.tv_sec - start.tv_sec + (done.tv_nsec - start.tv_nsec)/1000000000.0;
	return ret;
}

/**
 * time now + zeit in ms
 */
void setMSTimeout(timespec &timeout, int ms) {
	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_nsec += ms * 1000000;
	timeout.tv_sec += timeout.tv_nsec/1000000000; // macht sonst exception
	timeout.tv_nsec = timeout.tv_nsec%1000000000;
}

/**
 * ping thread
 */
static void *startCommThread(void *data)
{
	printf("start comm thread\n");

	bool forceRead=false;

	try {
	while(true) {
		struct timespec timeout,start,done;
		int retcode;
		// printf("X try lock\n");
		// pthread_mutex_lock(&mut);
		// printf("X locked\n");
		setMSTimeout(timeout, 100);

		clock_gettime(CLOCK_REALTIME, &start);
		retcode = 0;
		usleep(100000);
		printf("S "); fflush(stdout);
		/*
		while (x <= y && retcode == 0) {
			// printf("X cond_timedwait %d %d timeout:%ld\n", x, y, timeout.tv_nsec);
			retcode = pthread_cond_timedwait(&cond, &mut, &timeout);
		}
		*/
// FIXME: nach string senden IMMER einen leseversuch machen!
		clock_gettime(CLOCK_REALTIME, &done);
		// printf("X x:%d, y:%d, retcode=%s waittime=%g\n",x,y,strerror(retcode),timediff(start,done));
		if(x <= y || forceRead) {
			retcode=ETIMEDOUT;
			forceRead=false;
		}
		if (retcode == ETIMEDOUT) {
			// printf("X timeout\n");
			/* timeout occurred */
			/*
			unsigned char thechar=' ';
			// zeichen lesen wenn timeout
			int result = libusb_control_transfer(usbDigispark->devHandle, (0x01 << 5) | 0x80, 0x01, 0, 0, &thechar, 1, 1000);
			printf("X r:%c=%d rc:%d\n",thechar, thechar, result);
			*/
			while( read(usbDigispark->devHandle) ) {
				usleep(10000);
			}
			// pthread_mutex_unlock(&mut);
			// printf("X unlocked\n");
		} else if(retcode != 0) {
			printf("X retcode = %s\n", strerror(retcode));
			abort();
		} else {
			printf("X x > y\n");
			/* operate on x and y */
			// kein timeout also x > y => command im buffer:
			y++;
			pthread_mutex_lock(&mut);
			char buffer[1024];
			strcpy(buffer,commBuffer);
			pthread_mutex_unlock(&mut);
			// printf("X unlocked\n");
			char *pos=buffer;
			while(*pos != '\0') {
				printf("W " ANSI_RED1 "%c" ANSI_DEFAULT " ",*pos); fflush(stdout);
				// TODO: *pos == uint16_t wValue da könnt ma Mxx statt 4 bytes einzeln machen!
				int result = libusb_control_transfer(usbDigispark->devHandle, (0x01 << 5), 0x09, 0, *pos, 0, 0, 1000);
				//printf("Writing character \"%c\" to DigiSpark.\n", input[i]);
				if(result < 0) {
					printf("X Error %i writing to USB device [%s]\n", result, buffer);
					throw "error writing to usb device";
				}
				pos++;
			}
			pthread_mutex_lock(&buffer_empty_mut);
			printf("X signal buffer_empty_cond\n");
			pthread_cond_signal(&buffer_empty_cond);
			pthread_mutex_unlock(&buffer_empty_mut);

			forceRead=true;
		}
	}
	} catch(const char *errormsg) {
		printf("X exception %s\n", errormsg);
		abort();
	}
}

void sendCmd(const char *cmd) {
	// printf("S try lock\n");
	pthread_mutex_lock(&mut);
	// printf("S locked\n");
	strcpy(commBuffer,cmd);
	/* modify x and y */
	x++;
	if (x > y) {
		int rc=pthread_cond_broadcast(&cond);
		if(rc != 0) {
			printf("S error cond_broadcast %s\n", strerror(rc));
			abort();
		}
	}
	pthread_mutex_unlock(&mut);
	// printf("S unlocked\n");
}

USBDigispark::USBDigispark(int devnr, bool debug) :
	USBPlatine(debug), devHandle(NULL), dir(0), pwm(0) {

	// Initialize the USB library
	if(libusb_init(&context) < 0) {
		throw "could not initialize libusb";
	}

	try {
		this->init(devnr);
	} catch (...) {
		this->release();
		throw;
	}
}

USBDigispark::~USBDigispark() {
	this->fullstop();
	this->release();
}

void USBDigispark::release() {
	if(this->devHandle) {
		libusb_release_interface(this->devHandle, 0);
		libusb_close(this->devHandle);
		this->devHandle=NULL;
	}
	if(context) {
		libusb_exit(context);
		context=NULL;
	}
}

#define DIGISPARK_VENDOR_ID 0x16c0
#define DIGISPARK_PRODUCT_ID 0x05df

void USBDigispark::init(int devnr) {

	// Enumerate the USB device tree

	libusb_device **connected_devices = NULL;

	ssize_t size = libusb_get_device_list(context, &connected_devices); /* get all devices on system */
	if (size <= 0) {
		throw "no usb devices found on system";
	}

	libusb_device *digiSpark = NULL; /* device on port */

	for (ssize_t i = 0; i < size; ++i) { /* look for the device at given port */
		struct libusb_device_descriptor descriptor;
		libusb_get_device_descriptor(connected_devices[i], &descriptor);
		// printf("----- vendor:%#x product:%#x\n",descriptor.idVendor,descriptor.idProduct);
		if (descriptor.idVendor == DIGISPARK_VENDOR_ID
				&& descriptor.idProduct == (DIGISPARK_PRODUCT_ID ))
			digiSpark = connected_devices[i];
	}
	/*
	// Iterate through attached busses and devices
	bus = usb_get_busses();
	while(bus != NULL) {
		device = bus->devices;
		while(device != NULL) {
			// Check to see if each USB device matches the DigiSpark Vendor and Product IDs
			if((device->descriptor.idVendor == 0x16c0) && (device->descriptor.idProduct == 0x05df)) {
				digiSpark = device;
			}

			printf("found Digispark device");
			device = device->next;
			break;
		}

		bus = bus->next;
	}
	*/
	if(digiSpark == NULL) {
		throw "No Digispark Found";
	}
	int r = libusb_open(digiSpark, &this->devHandle);
	libusb_free_device_list(connected_devices, 1); /* we got the handle, free references to other devices */

	if (r == LIBUSB_ERROR_ACCESS) {
		throw "could not open device, you don't have the required permissions";
	} else if (r != 0) {
		throw "could not open device";
	}

	if (libusb_kernel_driver_active(this->devHandle, 0) == 1) { /* find out if kernel driver is attached */
		if (libusb_detach_kernel_driver(this->devHandle, 0) != 0) { /* detach it */
			throw "could not detach kernel driver";
		}
	}

	r = libusb_claim_interface(this->devHandle, 0); /* claim interface 0 (the first) of device */
	if (r != 0) {
		throw "could not claim interface";
	}

/*
	// nach interfaces schaun:
	int numInterfaces = 0;

	if(this->devHandle != NULL) {
		/ *result = usb_set_configuration(devHandle, digiSpark->config->bConfigurationValue);
		  if(result < 0) {printf("Error %i setting configuration to %i\n", result, digiSpark->config->bConfigurationValue); return 1;}* /

		numInterfaces = digiSpark->config->bNumInterfaces;
		this->interface = &(digiSpark->config->interface[0].altsetting[0]);
		//if(debug) printf("Found %i interfaces, using interface %i\n", numInterfaces, interface->bInterfaceNumber);

		/ *result = usb_claim_interface(devHandle, interface->bInterfaceNumber);
		  if(result < 0) { printf("Error %i claiming Interface %i\n", result, interface->bInterfaceNumber); return 1;}* /

	}
	*/

	if(useCommthread) {
		usbDigispark=this;
		void *startupData=NULL;
		bzero(&commThread,sizeof(commThread));
		if(int rc=pthread_create(&commThread, NULL, startCommThread, (void *)startupData) != 0) {
			printf("error creating new thread rc=%d\n",rc);
			perror("error creating new thread ");
			abort();
		}
		printf("new Thread: %lx\n",commThread);
		pthread_detach(commThread);
	}
	printf("digispark init done\n");
	this->setPWM(0);
}

void USBDigispark::setPWM(unsigned char pwm) {
	// int result = 0;
	if(this->pwm!=pwm) {
		char buffer[100];
		snprintf(buffer, sizeof(buffer), "M%02x\n", pwm);
		sendCmd(buffer);
		this->pwm=pwm;
	}
	/*
	char *pos=buffer;
	while(*pos != '\0') {
		printf("W "ANSI_RED1 "%c" ANSI_DEFAULT " ",*pos);
		// TODO: *pos == uint16_t wValue da könnt ma Mxx statt 4 bytes einzeln machen!
		result = libusb_control_transfer(this->devHandle, (0x01 << 5), 0x09, 0, *pos, 0, 0, 1000);
		//printf("Writing character \"%c\" to DigiSpark.\n", input[i]);
		if(result < 0) {
			printf("Error %i writing to USB device [pwm]\n", result);
			throw "error writing to usb device";
		}
		pos++;
    }
	printf("\n");
	*/
	// TODO: return einlesen!
}

void USBDigispark::setDir(unsigned char dir) {
	// int result = 0;
	if(this->dir!=dir) {
		char buffer[100];
		snprintf(buffer, sizeof(buffer), "D%d\n", dir);
		sendCmd(buffer);
		this->dir=dir;
	}
	/*
	char *pos=buffer;
	while(*pos != '\0') {
		printf("W "ANSI_RED1 "%c" ANSI_DEFAULT " ",*pos);
		result = libusb_control_transfer(this->devHandle, (0x01 << 5), 0x09, 0, *pos, 0, 0, 1000);
		//printf("Writing character \"%c\" to DigiSpark.\n", input[i]);
		if(result < 0) {
			printf("Error %i writing to USB device\n", result);
			throw "error writing to usb device [dir]";
		}
		pos++;
    }
	printf("\n");
	*/
	// TODO: return einlesen!
}

void USBDigispark::fullstop() {
	printf("USBDigispark::fullstop()\n");
	setPWM(0);
	// 1s warten dass das command fertig is:
	struct timespec timeout;
	setMSTimeout(timeout, 1000);
	pthread_mutex_lock(&buffer_empty_mut);
	int retcode = pthread_cond_timedwait(&buffer_empty_cond, &buffer_empty_mut, &timeout);
	printf("USBDigispark::fullstop() retcode=%s\n",strerror(retcode));
	pthread_mutex_unlock(&buffer_empty_mut);

	// nachdem wir nix reinschreiben wollen noch einem anderen eventuell wartenden thread signalisieren dass ma in buffer schreiben kann:
	pthread_mutex_lock(&buffer_empty_mut);
	pthread_cond_signal(&buffer_empty_cond);
	pthread_mutex_unlock(&buffer_empty_mut);
	printf("USBDigispark::fullstop() done\n");
}

const char *USBDigispark::readLine() {
	unsigned char thechar=' ';
	int result;
	unsigned int pos=0;
	while(thechar != 4) {
		result = libusb_control_transfer(this->devHandle, (0x01 << 5) | 0x80, 0x01, 0, 0, &thechar, 1, 1000);
		if(result <= 0) {
			break;
		}
//		thechar != '\n' && 
		if(pos > sizeof(this->buffer)-1) {
			printf("buffer overrun\n");
			break;
		}
		printf("readLine: char %c=%#x\n", thechar,thechar);
		buffer[pos++]=thechar;
		usleep(100000);

	}
	buffer[pos]='\0';
	return buffer;
}
/*
    int i = 0;
    int a = 0;
    int stringLength;

   while (c != 3) { 
		c = getch();      // retrieve xtended scancode
		if(c != ERR){

			if (c == 10){

				if(sendLine)
					input[a] = '\n';

				a++;
				stringLength = a;
				i=0;

				int y, x;
				getyx(childwin, y, x);


				if(x!=0)
					wprintw(childwin, "\n");

				waddstr(childwin, ">");
				while(i < stringLength){
					wprintw(childwin, "%c", input[i]);
	            	result = usb_control_msg(devHandle, (0x01 << 5), 0x09, 0, input[i], 0, 0, 1000);
	            	//printf("Writing character \"%c\" to DigiSpark.\n", input[i]);
	            	if(result < 0) {printf("Error %i writing to USB device\n", result); return 1;}
	            	i++;
	          	}

	          	if(!sendLine)
	          		wprintw(childwin, "\n");

				//
				

				a=0;
	      input[0] = '\0';


				
				
			}
			else{

				input[a] = c;
				a++;
				wprintw(inputwin, "%c", c);
				wrefresh(inputwin);

			}
		}
		/ *
		else {
			thechar = ' ';

			while(thechar != 4)
			{

				thechar = 4;
				result = usb_control_msg(devHandle, (0x01 << 5) | 0x80, 0x01, 0, 0, &thechar, 1, 1000);
				if(result > 0)
				{
					if(thechar != 4){
						if(thechar=='\n'){
							int y, x;
							getyx(childwin, y, x);

							if(y==LINES-4)
								wscrl(childwin,1);
							else
								result = wmove(childwin, y + 1, x);
						}
						else{
							waddch(childwin, thechar);

						}



					}
				}
				usleep(100000);
			}
			wrefresh(childwin);
			refresh();

		}

	}



    //printf("received %c (%d)\n", c, (int) c);
}
*/

char read(libusb_device_handle *devHandle) {
	// rc=test.readLine();
	unsigned char thechar='\0';
	int result = libusb_control_transfer(devHandle, (0x01 << 5) | 0x80, 0x01, 0, 0, &thechar, 1, 1000);
	if(result < 0) {
		printf("error reading: %d errno=%s\n", result, strerror(errno));
	}
	if(result == 0) { 
		printf("R");
	} else if(thechar < ' ') {
		printf("r:%d ",thechar);
	} else {
		printf("r:" ANSI_GREEN1 "%c" ANSI_DEFAULT " ",thechar);
	}
	fflush(stdout);
	return thechar;
}


#ifdef DIGISPARK_TEST
#include <unistd.h>
int main(int argc, char* argv[]) {
	printf("Digispark test mode\n");
	USBDigispark test(0,true);
	// const char *rc;
	while(1) {
		printf("S while 1\n");
		if(useCommthread) {
			sendCmd("M65\n");
			sleep(1);
			printf("S while 2\n");
			sendCmd("M00\n");
			sleep(1);
		} else {
			test.setPWM(99);
			usleep(100000);
			// rc=test.readLine();
			// printf("result: %s\n", rc);
			for(int i=0; i < 10; i++) {
				read(test.devHandle);
				usleep(100000);
			}
			printf("\n");
			test.setPWM(0);
			usleep(100000);
			// rc=test.readLine();
			// printf("result: %s\n", rc);
			for(int i=0; i < 10; i++) {
				read(test.devHandle);
				usleep(100000);
			}
		}
		printf("\n");
		
	}

}
#endif
