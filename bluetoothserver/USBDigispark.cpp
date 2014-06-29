/*
 * aus digispark.c monitor
 */
#include <stdio.h>
#include <string.h>
#include <libusb-1.0/libusb.h>        // libusb-1.0
#include <stdlib.h>
#include "USBDigispark.h"

static libusb_context* context = NULL;

USBDigispark::USBDigispark(int devnr, bool debug) :
	USBPlatine(debug), devHandle(NULL), dir(0) {

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

	printf("digispark init done\n");
}

void USBDigispark::setPWM(unsigned char pwm) {
	int result = 0;
	char buffer[100];
	snprintf(buffer, sizeof(buffer), "M%02x\n", pwm);
	char *pos=buffer;
	while(*pos != '\0') {
		printf("USBDigispark write char %c\n",*pos);
		// TODO: *pos == uint16_t wValue da könnt ma Mxx statt 4 bytes einzeln machen!
		result = libusb_control_transfer(this->devHandle, (0x01 << 5), 0x09, 0, *pos, 0, 0, 1000);
		//printf("Writing character \"%c\" to DigiSpark.\n", input[i]);
		if(result < 0) {
			printf("Error %i writing to USB device [pwm]\n", result);
			throw "error writing to usb device";
		}
		pos++;
    }
	// TODO: return einlesen!
}

void USBDigispark::setDir(unsigned char dir) {
	int result = 0;
	char buffer[100];
	snprintf(buffer, sizeof(buffer), "D%d\n", dir);
	char *pos=buffer;
	while(*pos != '\0') {
		printf("USBDigispark write char %c\n",*pos);
		result = libusb_control_transfer(this->devHandle, (0x01 << 5), 0x09, 0, *pos, 0, 0, 1000);
		//printf("Writing character \"%c\" to DigiSpark.\n", input[i]);
		if(result < 0) {
			printf("Error %i writing to USB device\n", result);
			throw "error writing to usb device [dir]";
		}
		pos++;
    }
	// TODO: return einlesen!
}

void USBDigispark::fullstop() {
	setPWM(0);
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
