/***************************************************************************
 *   Copyright (C) 2004 by Nicolas Sutre                                   *
 *   nicolas.sutre@free.fr                                                 *
 *                                                                         *
 *   Copyright (C) 2005 by Bob Dempsey                                     *
 *   bdempsey_64@msn.com                                                   *
 *                                                                         *
 *   Copyright (C) 2005 by Julien Etelain and Edward Nys                   *
 *   Converted to C                                                        *
 *   Commented and improved by Julien Etelain <julien.etelain@utbm.fr>     *
 *                             Edward Nys <edward.ny@utbm.fr>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <usb.h>
#include <assert.h>
#include <sys/time.h>
// fürs sleep
#include <unistd.h>
#include <errno.h>
    
#include "k8055.h"

#define STR_BUFF 256
#define false 0
#define true 1

#define CMD_INIT		0x00
#define CMD_SETDBT1		0x01
#define CMD_SETDBT2		0x02
#define CMD_RESETCNT1   0x03
#define CMD_RESETCNT2   0x04
#define CMD_SETOUTPUT   0x05

#define PACKET_LEN 8
#define USB_TIMEOUT 100
#define USB_OUT_EP 0x01	/* USB output endpoint */
#define USB_INP_EP 0x81 /* USB Input endpoint */


K8055::K8055(int ipid, bool debug):debug(debug),
	xsv_handle(NULL)

{
	/*
		Initialise USB system
		and enable debug mode
	*/
	usb_init();
	
	switch (ipid) // Board address
	{
		case 1 : this->ipid = 0x5500; break;
		case 2 : this->ipid = 0x5501; break;
		case 3 : this->ipid = 0x5502; break;
		case 4 : this->ipid = 0x5503; break;
		default : fprintf(stderr,"invalid id"); assert(0);
	}

	if ( debug ) 
		usb_set_debug(2);
	
	/*
		Search for the device
	*/
	if ( !(xsv_handle = locate_xsv()) ) {
		throw "Could not find the XSV device\nPlease ensure that the device is correctly connected.";
		
	} else if ( !takeover_device( 0 ) )	{
	
		usb_close(xsv_handle);
		throw "takeover device failed";
		
	} else {

		write_empty_command(CMD_INIT);
	}
		
	this->write_output(0, 0, 0x01);
}

K8055::~K8055()
{
	printf("platine -> write_out 0 0 0x0a\n");
	this->write_output(0, 0, 0x02);
	unsigned char a1, a2, d; short unsigned int c1, c2;
	this->read_input(&a1, &a2, &d, &c1, &c2 ); // irgendwas einlesen - write_output und dann gleich ein close => kommt nie an
	printf("read: pwm1: %u, pwm2:%u, d:%x, counter1:%u, counter2:%u\n", a1, a2, d, c1, c2);
	usb_close(xsv_handle);
}

int K8055::takeover_device( int interface )
{

/*
	Only avaible on linux
*/
#if defined(LIBUSB_HAS_GET_DRIVER_NP) && defined(LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP)

	char driver_name[STR_BUFF];
	/*
		Clear buffer
	*/
	memset( driver_name, 0, STR_BUFF );
	
	/*
		Ensure that the handle is not null, else program exists
	*/
	assert( xsv_handle != NULL ); 
     
    /*
    	Read driver name, if found display it 
    */
	if ( usb_get_driver_np( xsv_handle, interface, driver_name, sizeof(driver_name) ) < 0 )
		if ( debug ) 
			fprintf( stderr, "get driver name: %s\n", usb_strerror());
	   
	/*
		Disconnect eventual driver yet connected to device.
	*/ 
	if ( 0 > usb_detach_kernel_driver_np( xsv_handle, interface ) ) {
		if ( debug ) 
			fprintf( stderr, "Disconnect OS driver: %s\n", usb_strerror());
	} else
		if ( debug ) 
			fprintf( stderr, "Disconnected OS driver: %s\n", usb_strerror());
#else
	/*
		Ensure that the handle is not null, else program exists
	*/
	assert( xsv_handle != NULL ); 

#endif	

	/*
		Set device configuration (conforming to vellman protocol)
	*/
 	usb_set_configuration(xsv_handle, 1);
	
	/*
		Try to get exclusive rights on the device
	*/
	if( usb_claim_interface(xsv_handle, interface) <0 ) {
	
		fprintf( stderr,"%s\nif prog is runnung as non-root chmod -R a+w /dev/bus/usb/%s/%s\nmight help\n",
			usb_strerror(),cfg_devHandle->bus->dirname,cfg_devHandle->filename);
		return false;
		
	} else if ( debug ) 
		fprintf( stderr,"Find interface %d\n", interface);
      
	if ( debug ) 
		fprintf( stderr, "Took over the device\n");
  
	return true;
}


/*
	Search the XSV device, and open it
	
	Return the handle on sucess
			else return NULL
*/
usb_dev_handle *K8055::locate_xsv(void)
{
	struct usb_bus *bus;
	struct usb_device *dev;
	usb_dev_handle *device_handle = 0;

	usb_find_busses();
	usb_find_devices();

 	for (bus = usb_busses; bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			if ( (dev->descriptor.idVendor == 0x10cf) && 
			     (dev->descriptor.idProduct == ipid ) ) {
			
				device_handle = usb_open(dev);
				cfg_devHandle=dev;
				
				if ( debug ) 
					fprintf(stderr,"XSV Device Found @ Address %s Vendor 0x0%x Product ID 0x0%x\n", dev->filename, dev->descriptor.idVendor,dev->descriptor.idProduct);

				return device_handle;
			}
		}
	}
	
	return NULL;
} 

/*
	Function to send a command without parameters
	command=CMD_INIT	Initlialise device
	command=CMD_RESETCNT1   Set to 0 the counter 1
	command=CMD_RESETCNT2   Set to 0 the counter 2	
*/
int K8055::write_empty_command ( unsigned char command )
{
	unsigned char data[8];
	memset(	data,0,8);								

	data[0] = command;
	
	if ( debug )
		fprintf(stderr,"write_empty_command(0x%p,0x%x);\n",xsv_handle,command);
	
	if (usb_interrupt_write(xsv_handle,0x1, (char*) data,PACKET_LEN,20) )
		return false;
		
	return true;
}

/*
	Function to send DebounceTime commands (CMD_SETDBT1,CMD_SETDBT2)
	command=CMD_SETDBT1 used to set debounce time for counter 1
	command=CMD_SETDBT2 used to set debounce time for counter 1
	
	ATTENTION : t1 and t2 values are not msec !
		use msec_to_dbt_code
	
*/

int K8055::write_dbt_command ( unsigned char command, unsigned char t1, unsigned char t2 )
{
	unsigned char data[8];
	memset(	data,0,8);								

	data[0] = command;
	data[6] = t1;
	data[7] = t2;
	
	if ( debug )
		fprintf(stderr,"write_dbt_command(0x%p,0x%x,%d,%d);\n",xsv_handle,command,t1,t2);

	
	if ( 0 > usb_interrupt_write(xsv_handle,0x1, (char*) data,PACKET_LEN,20) )
		return false;
		
	return true;
}

/**
 *	Write output values
 */
int K8055::write_output ( unsigned char a1, unsigned char a2, unsigned char d ) {
	unsigned char data[8];
	memset(	data,0,8);

	data[0] = CMD_SETOUTPUT;
	data[1] = d;
	data[2] = a1;
	data[3] = a2;
	
	if ( debug )
		fprintf(stderr,"write_dbt_command(0x%p,%d,%d,0x%x);\n",xsv_handle,a1,a2,d);
		
	int rc;
	for(int i=0; i < 3; i++) {
		rc=usb_interrupt_write(xsv_handle,0x1, (char*) data,PACKET_LEN,20);
		if(rc == PACKET_LEN) {
			return true;
		}
		printf("K8055::write_output error %s\n",strerror(errno));
		usleep(200000);
	}
	return false;
		
}

/*
	Read input values
	@param a1	-> wert analog in 1
	@param a2	-> wert analog in 2
	@param d	-> wert digital out
	@param c1	-> counter1
	@param c2	-> counter2
*/
int K8055::read_input ( unsigned char *a1, unsigned char *a2, unsigned char *d, unsigned short *c1, unsigned short *c2 ) {
	unsigned char data[8];

	int rc;
	for(int i=0; i < 3; i++) {
		rc=usb_interrupt_read(xsv_handle, USB_INP_EP, (char*) data,PACKET_LEN,USB_TIMEOUT);
		if(rc == PACKET_LEN) {
			goto weiter;
		}
		printf("K8055::read_input error reading %dbytes [%d=%s]\n",PACKET_LEN,rc,strerror(errno));
		usleep(200000);
	}
	return false;
weiter:
	if ( a1 )
		*a1 = data[2];
		
	if ( a2 )
		*a2 = data[3];
		
	if ( d ) {
		*d = ((data[0 ]& 0x10) >> 4) | ((data[0] & 0x20) >> 4) | ((data[0] & 0x1) << 2) |
			 ((data[0 ]& 0x40) >> 3) | ((data[0] & 0x80) >> 3);
	}
	
	if ( c1 ) 
		*c1 = data[4] | data[5]<<8;
		
	if ( c2 ) 
		*c2 = data[6] | data[7]<<8;
		
	return true;
}

/*
	Function to convert msec to the DebounceTime encoding
	ATTENTION : Empirical values
*/
int K8055::msec_to_dbt_code ( int msec ) {

	if (msec < 2)
		return 1;
	else if (msec < 10)
		return 3;
	else if (msec < 1000)
		return 8;
	else if (msec < 5000)
		return 88;
	else
		return 255;
}

/*
	Set bounce time of counter number 1
*/
int K8055::set_counter1_bouncetime ( int msec ) {
	return write_dbt_command (CMD_SETDBT1, msec_to_dbt_code(msec), 0 );
}

/*
	Set bounce time of counter number 2
*/
int K8055::set_counter2_bouncetime ( int msec ) {
	return write_dbt_command (CMD_SETDBT2, 0, msec_to_dbt_code(msec));
}

/*
	Set to 0 counter 1
*/
int K8055::reset_counter1 ( ) {
	return write_empty_command(CMD_RESETCNT1);
}

/*
	Set to 0 counter 2
*/
int K8055::reset_counter2 ( ) {
	return write_empty_command(CMD_RESETCNT2);
}
