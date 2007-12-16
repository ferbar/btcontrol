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
     
#define STR_BUFF 256
#define false 0
#define true 1

#define CMD_INIT		0x00
#define CMD_SETDBT1		0x01
#define CMD_SETDBT2		0x02
#define CMD_RESETCNT1   0x03
#define CMD_RESETCNT2   0x04
#define CMD_SETOUTPUT   0x05

usb_dev_handle *locate_xsv(void);
struct usb_device *cfg_devHandle;

int ia1 = 0;
int ia2 = 0;
int id8 = 0;
int ipid = 1;

int numread = 1;

int debug = 1;

int dbt1 = -1; // (-1 => not to set)
int dbt2 = -1; // (-1 => not to set)

int resetcnt1 = false;
int resetcnt2 = false;

int delay = 0;

static int takeover_device( usb_dev_handle* udev, int interface )
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
	assert( udev != NULL ); 
     
    /*
    	Read driver name, if found display it 
    */
	if ( usb_get_driver_np( udev, interface, driver_name, sizeof(driver_name) ) < 0 )
		if ( debug ) 
			fprintf( stderr, "get driver name: %s\n", usb_strerror());
	   
	/*
		Disconnect eventual driver yet connected to device.
	*/ 
	if ( 0 > usb_detach_kernel_driver_np( udev, interface ) ) {
		if ( debug ) 
			fprintf( stderr, "Disconnect OS driver: %s\n", usb_strerror());
	} else
		if ( debug ) 
			fprintf( stderr, "Disconnected OS driver: %s\n", usb_strerror());
#else
	/*
		Ensure that the handle is not null, else program exists
	*/
	assert( udev != NULL ); 

#endif	

	/*
		Set device configuration (conforming to vellman protocol)
	*/
 	usb_set_configuration(udev, 1);
	
	/*
		Try to get exclusive rights on the device
	*/
	if( usb_claim_interface(udev, interface) <0 ) {
	
		fprintf( stderr,"%s\nif prog is runnung as non-root chmod -R a+w /dev/bus/usb/%s/\nmight help\n",
			usb_strerror(),cfg_devHandle->filename);
		return false;
		
	} else if ( debug ) 
		fprintf( stderr,"Find interface %s\n", interface);
      
	if ( debug ) 
		fprintf( stderr, "Took over the device\n");
  
	return true;
}


/*
	Search the XSV device, and open it
	
	Return the handle on sucess
			else return NULL
*/
usb_dev_handle *locate_xsv(void)
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
	Convert a string on n chars to an integer
	Return  1 on sucess
			0 on failure (non number)
*/
int Convert_StringToInt(char *text, int *i)
{
	return sscanf(text, "%d", i);
}

/*
	Write help to standard output
*/
void display_help ( char *params[] ) {

	printf("K8055 version 0.3 UTBM Build\n");
	printf("Copyright (C) 2004 by Nicolas Sutre\n");
	printf("Copyright (C) 2005 by Bob Dempsey\n");
	printf("Copyright (C) 2005 by Julien Etelain and Edward Nys\n");
	printf("\n");
	printf("Syntax : %s [-P:(number)] [-D:(value)] [-A1:(value)] [-A2:(value)] [-NUM:(number) [-DELAY:(number)]]\n",params[0]);
	printf(" [-DBT1:(value)] [-BDT2:(value)] [-reset1] [-reset2] [-debug]\n");
	printf("	-P:(number)		Set board number\n");
	printf("	-D:(value)		Set digital output value (8 bits in decimal)\n");
	printf("	-A1:(value)		Set analog output 1 value (0-255)\n");
	printf("	-A2:(value)		Set analog output 2 value (0-255)\n");
	printf("	-NUM:(number)   Set number of measures\n");	
	printf("	-DELAY:(number) Set delay between two measures (in msec)\n");		
	printf("	-DBT1:(value)   Set debounce time for counter 1 (in msec)\n");	
	printf("	-DBT2:(value)   Set debounce time for counter 2 (in msec)\n");	
	printf("	-reset1			Reset counter 1\n");
	printf("	-reset2			Reset counter 2\n");
	printf("	-debug			Activate debug mode\n");
	printf("Exemple : %s -P:1 -D:147 -A1:25 -A2:203\n",params[0]);
	printf("\n");
	printf("Output : (timestamp);(digital);(analog 1);(analog 2);(counter 1);(counter 2)\n");
	printf("Note : timestamp is the number of msec when data is read since program start\n");
	printf("Example : 499;16;128;230;9;8\n");
	printf("499 : Measure done 499 msec after program start\n");
	printf("16  : Digital input value is 10000 (I5=1, all other are 0)\n");
	printf("128 : Analog 1 input value is 128\n");
	printf("230 : Analog 2 input value is 230\n");
	printf("9   : Counter 1 value is 9\n");
	printf("8   : Counter 2 value is 8\n");

}


/*
	Read arguments, and store values
	Return true if arguments are valid
		else return false
*/
int read_param(int argc,char *params[])
{ 
	int erreurParam = false;
	int i;
  	
	ipid = 1;
  		
	for (i=1; i<argc;i++)
	{
		if ( !strncmp(params[i],"-P:",3) &&
		     !Convert_StringToInt(params[i]+3,&ipid) ) erreurParam = true;
		else
			if ( !strncmp(params[i],"-A1:",4)  &&
		    	!Convert_StringToInt(params[i]+4,&ia1) ) erreurParam = true;
		else
			if ( !strncmp(params[i],"-A2:",4) &&
		    	!Convert_StringToInt(params[i]+4,&ia2) ) erreurParam = true;
		else
			if ( !strncmp(params[i],"-D:",3) &&
		    	!Convert_StringToInt(params[i]+3,&id8) ) erreurParam = true;
		else
			if ( !strncmp(params[i],"-NUM:",5) &&
		    	!Convert_StringToInt(params[i]+5,&numread) ) erreurParam = true;	
		else
			if ( !strncmp(params[i],"-DELAY:",7) &&
		    	!Convert_StringToInt(params[i]+7,&delay) ) erreurParam = true;					
		else
			if ( !strncmp(params[i],"-DBT1:",6) &&
		    	!Convert_StringToInt(params[i]+6,&dbt1) ) erreurParam = true;						
			else
			if ( !strncmp(params[i],"-DBT2:",6) &&
		    	!Convert_StringToInt(params[i]+6,&dbt2) ) erreurParam = true;											
		else
			if ( !strcmp(params[i],"-debug") ) debug = true;
		else
			if ( !strcmp(params[i],"-reset1") ) resetcnt1 = true;
		else
			if ( !strcmp(params[i],"-reset2") ) resetcnt2 = true;					
		else
			if ( !strcmp(params[i],"--help") || !strcmp(params[i],"-h") ) {
				display_help(params);
				return false;
			}

	}
	
	
	/*
		Send parameters to standard error
	*/
	if ( debug )
		fprintf(stderr,"Parameters : Card=%d Analog1=%03d Analog2=%03d Digital=%03d\n",ipid,ia1,ia2,id8);
	
	switch (ipid) // Board address
	{
		case 1 : ipid = 0x5500; break;
		case 2 : ipid = 0x5501; break;
		case 3 : ipid = 0x5502; break;
		case 4 : ipid = 0x5503; break;
		default : erreurParam = true;
	}
		
	if (erreurParam) 
	{

		printf("Invalid or incomplete options\n");
		
		display_help(params);
		return false;
	} 


	return true; 
}

/*
	Function to send a command without parameters
	command=CMD_INIT	Initlialise device
	command=CMD_RESETCNT1   Set to 0 the counter 1
	command=CMD_RESETCNT2   Set to 0 the counter 2	
*/
int write_empty_command ( struct usb_dev_handle *xsv_handle, unsigned char command ) {
	unsigned char data[8];
	memset(	data,0,8);								

	data[0] = command;
	
	if ( debug )
		fprintf(stderr,"write_empty_command(0x%x,0x%x);\n",xsv_handle,command);
	
	if ( 0 > usb_interrupt_write(xsv_handle,0x1,data,8,20) )
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

int write_dbt_command ( struct usb_dev_handle *xsv_handle, unsigned char command, unsigned char t1, unsigned char t2 ) {
	unsigned char data[8];
	memset(	data,0,8);								

	data[0] = command;
	data[6] = t1;
	data[7] = t2;
	
	if ( debug )
		fprintf(stderr,"write_dbt_command(0x%x,0x%x,%d,%d);\n",xsv_handle,command,t1,t2);

	
	if ( 0 > usb_interrupt_write(xsv_handle,0x1,data,8,20) )
		return false;
		
	return true;
}

/*
	Write output values
*/
int write_output ( struct usb_dev_handle *xsv_handle, unsigned char a1, unsigned char a2, unsigned char d ) {
	unsigned char data[8];
	memset(	data,0,8);								

	data[0] = CMD_SETOUTPUT;
	data[1] = d;
	data[2] = a1;
	data[3] = a2;
	
	if ( debug )
		fprintf(stderr,"write_dbt_command(0x%x,%d,%d,0x%x);\n",xsv_handle,a1,a2,d);
		
	if ( 0 > usb_interrupt_write(xsv_handle,0x1,data,8,20) )
		return false;
		
	return true;
}

/*
	Read input values
*/
int read_input ( struct usb_dev_handle *xsv_handle, unsigned char *a1, unsigned char *a2, unsigned char *d, unsigned short *c1, unsigned short *c2 ) {
	unsigned char data[8];

	if ( 0 > usb_interrupt_read(xsv_handle,0x81,data,8,20) )
		return false;

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
		
}

/*
	Function to convert msec to the DebounceTime encoding
	ATTENTION : Empirical values
*/
int msec_to_dbt_code ( int msec ) {

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
int set_counter1_bouncetime ( struct usb_dev_handle *xsv_handle, int msec ) {
	return write_dbt_command (xsv_handle, CMD_SETDBT1, msec_to_dbt_code(msec), 0 );
}

/*
	Set bounce time of counter number 2
*/
int set_counter2_bouncetime ( struct usb_dev_handle *xsv_handle, int msec ) {
	return write_dbt_command (xsv_handle, CMD_SETDBT2, 0, msec_to_dbt_code(msec));
}

/*
	Set to 0 counter 1
*/
int reset_counter1 ( struct usb_dev_handle *xsv_handle ) {
	return write_empty_command(xsv_handle,CMD_RESETCNT1);
}

/*
	Set to 0 counter 2
*/
int reset_counter2 ( struct usb_dev_handle *xsv_handle ) {
	return write_empty_command(xsv_handle,CMD_RESETCNT2);
}

/*
	Give current timestamp in miliseconds
*/
inline unsigned long int time_msec ( void ) {
	struct timeval t; struct timezone tz;
	gettimeofday (&t,&tz);
	return (1000*t.tv_sec)+(t.tv_usec/1000);
}


int main (int argc,char *params[]) 
{
	int i;
	char c;
	struct usb_dev_handle *xsv_handle;
	unsigned char a1, a2, d;
	unsigned short c1, c2;
	unsigned long int start,mstart=0,lastcall=0;

	start = time_msec();

	/*
		Load parameters
		If parameters are valid continue
	*/
	
	if (read_param(argc,params)) 
	{
		/*
			Initialise USB system
			and enable debug mode
		*/
		usb_init();
		
		if ( debug ) 
			usb_set_debug(2);

		/*
			Search for the device
		*/
		if ( !(xsv_handle = locate_xsv()) ) {
			printf("Could not find the XSV device\nPlease ensure that the device is correctly connected.\n");
			return (-1);
			
		} else if ( !takeover_device(xsv_handle, 0 ) )	{
		
			usb_close(xsv_handle);
			return (-1);
			
		} else {

			write_empty_command(xsv_handle,CMD_INIT);
			
			if ( resetcnt1 )
				reset_counter1(xsv_handle);
			if ( resetcnt2 )
				reset_counter2(xsv_handle);
				
			if ( dbt1 != -1 )
				set_counter1_bouncetime(xsv_handle,dbt1);
				
			if ( dbt2 != -1 )
				set_counter2_bouncetime(xsv_handle,dbt2);
				
			mstart = time_msec(); // Measure start
			for (i=0; i<numread; i++) {
				
				if ( delay ) {
					// Wait until next measure
					while ( time_msec()-mstart < i*delay );
				} 
				read_input ( xsv_handle,&a1, &a2, &d, &c1, &c2 );
				lastcall = time_msec();
				printf("%d;%d;%d;%d;%d;%d\n", (int)(lastcall-start),d, a1,a2,c1,c2 );
			}
			
			write_output(xsv_handle, ia1,ia2,id8);
						
			usb_close(xsv_handle);
		}
	}
}
